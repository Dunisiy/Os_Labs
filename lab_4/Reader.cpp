#include "Shared.h"

#include <mmsystem.h>
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#pragma comment(lib, "winmm.lib")

HANDLE hFileMap = NULL;
LPVOID pBase = NULL;

SharedMeta* pMeta = NULL;
char* pPages = NULL;

HANDLE hMutexState[NUM_PAGES] = { NULL };
HANDLE hCanWrite[NUM_PAGES] = { NULL };
HANDLE hCanRead[NUM_PAGES] = { NULL };

DWORD myPid = 0;

SIZE_T g_MapSize = 0;
DWORD g_PageSize = 0;

std::ofstream logFile;

void Log(const std::string& state, int page = -1)
{
	DWORD t = timeGetTime();

	std::string msg =
		std::to_string(t) +
		" | READER_" +
		std::to_string(myPid) +
		" | " +
		state;

	if (page >= 0)
		msg += " | PAGE " + std::to_string(page);

	std::cout << msg << std::endl;

	if (logFile.is_open())
	{
		logFile << msg << std::endl;
		logFile.flush();
	}
}

void Cleanup()
{
	if (pBase)
	{
		VirtualUnlock(pBase, g_MapSize);
		UnmapViewOfFile(pBase);
	}

	for (int i = 0; i < NUM_PAGES; i++)
	{
		if (hMutexState[i])
			CloseHandle(hMutexState[i]);

		if (hCanWrite[i])
			CloseHandle(hCanWrite[i]);

		if (hCanRead[i])
			CloseHandle(hCanRead[i]);
	}

	if (hFileMap)
		CloseHandle(hFileMap);

	if (logFile.is_open())
		logFile.close();
}

void ReadPage(int pageIdx)
{
	char* pagePtr =
		pPages + (SIZE_T)pageIdx * g_PageSize;

	char buffer[65];

	memcpy(buffer, pagePtr, 64);

	buffer[64] = '\0';

	std::cout
		<< "PAGE[" << pageIdx << "] => "
		<< buffer
		<< std::endl;
}

int main(int argc, char* argv[])
{
	if (argc < 2) { std::cout << "Usage: Reader.exe <start_page>\n"; return 1; }

	myPid = GetCurrentProcessId();
	int startPage = atoi(argv[1]) % NUM_PAGES;

	std::string logName = "C:\\Users\\denis\\source\\repos\\GIT_ETU_4314\\OS\\lab\\Kuznetsov_DS\\lab_4\\temp\\Reader_" +
		std::to_string(myPid) + ".log";
	logFile.open(logName);
	if (!logFile.is_open()) std::cerr << "Cannot open reader log file\n";

	SYSTEM_INFO si; GetSystemInfo(&si);
	g_PageSize = si.dwPageSize;
	g_MapSize = sizeof(SharedMeta) + (SIZE_T)NUM_PAGES * g_PageSize;
	SetProcessWorkingSetSize(GetCurrentProcess(), 256 * 1024 * 1024, 512 * 1024 * 1024);

	hFileMap = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"Global\\RW_LAB4_MEMORY");
	if (!hFileMap) { std::cerr << "Shared memory not found. Run Writer first.\n"; return 1; }

	pBase = MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (!pBase) { std::cerr << "MapViewOfFile failed\n"; Cleanup(); return 1; }

	VirtualLock(pBase, g_MapSize);
	pMeta = (SharedMeta*)pBase;
	pPages = (char*)pBase + sizeof(SharedMeta);

	for (int i = 0; i < NUM_PAGES; i++) {
		hMutexState[i] = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, (L"Global\\RW_MUTEX_" + std::to_wstring(i)).c_str());
		hCanWrite[i] = OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, FALSE, (L"Global\\RW_CAN_WRITE_" + std::to_wstring(i)).c_str());
		hCanRead[i] = OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, FALSE, (L"Global\\RW_CAN_READ_" + std::to_wstring(i)).c_str());
		if (!hMutexState[i] || !hCanWrite[i] || !hCanRead[i]) { i--; Sleep(10); }
	}

	srand((unsigned)time(NULL) + myPid);
	Log("START");

	int pageIdx = startPage;
	while (true)
	{
		pageIdx %= NUM_PAGES;

		// 1. Проверка завершения (учитывает флаг писателей)
		bool allDone = true;
		for (int i = 0; i < NUM_PAGES; i++) {
			bool readOk = (pMeta->readCount[i] >= MAX_WRITES_PER_PAGE);
			bool finishedOk = (pMeta->writersFinished && pMeta->readCount[i] >= pMeta->writeCount[i]);
			if (!readOk && !finishedOk) { allDone = false; break; }
		}
		if (allDone) { Log("FINISHED"); break; }

		Log("WAIT_READ", pageIdx);

		// Короткий таймаут позволяет периодически проверять allDone
		if (WaitForSingleObject(hCanRead[pageIdx], 100) == WAIT_TIMEOUT) {
			pageIdx++; continue;
		}

		WaitForSingleObject(hMutexState[pageIdx], INFINITE);

		// 🔥 КЛЮЧЕВОЕ ИСПРАВЛЕНИЕ: Синхронизация при завершении
		//if (pMeta->writersFinished && !pMeta->hasData[pageIdx]) {
		//	if (pMeta->readCount[pageIdx] < pMeta->writeCount[pageIdx]) {
		//		pMeta->readCount[pageIdx] = pMeta->writeCount[pageIdx];
		//		Log("SYNC_READ_COUNT", pageIdx);
		//	}
		//	ReleaseMutex(hMutexState[pageIdx]);
		//	pageIdx++; continue;
		//}

		if (!pMeta->hasData[pageIdx]) {
			ReleaseMutex(hMutexState[pageIdx]);
			pageIdx++; continue;
		}

		// 2. Регистрация читателя
		pMeta->readerCount[pageIdx]++;
		LONG currentReaders = pMeta->readerCount[pageIdx];
		if (currentReaders < MAX_CONCURRENT_READERS) {
			ReleaseSemaphore(hCanRead[pageIdx], 1, NULL);
		}
		ReleaseMutex(hMutexState[pageIdx]);

		// 3. Чтение (мьютекс отпущен → читатели работают параллельно)
		Log("READING", pageIdx);
		DWORD duration = 500 + rand() % 1001;
		Sleep(duration);
		ReadPage(pageIdx);

		// 4. Снятие регистрации
		WaitForSingleObject(hMutexState[pageIdx], INFINITE);
		pMeta->readerCount[pageIdx]--;
		LONG readersLeft = pMeta->readerCount[pageIdx];

		// 5. ПОСЛЕДНИЙ ЧИТАТЕЛЬ разрешает перезапись
		if (readersLeft == 0) {
			pMeta->readCount[pageIdx]++;
			pMeta->hasData[pageIdx] = FALSE;

			//WaitForSingleObject(hCanRead[pageIdx], 0); // Очистка лишних токенов
			ReleaseSemaphore(hCanWrite[pageIdx], 1, NULL); // 🔥 Открываем страницу для писателя
			Log("LAST_READER", pageIdx);
		}
		ReleaseMutex(hMutexState[pageIdx]);
		Log("RELEASE", pageIdx);
		pageIdx++;
	}

	Cleanup();
	return 0;
}