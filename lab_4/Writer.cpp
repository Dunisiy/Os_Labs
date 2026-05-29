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
		" | WRITER_" +
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

void WritePage(int pageIdx)
{
	char* pagePtr = pPages + (SIZE_T)pageIdx * g_PageSize;

	std::string text =
		"WRITER PID = " +
		std::to_string(myPid) +
		" PAGE = " +
		std::to_string(pageIdx) +
		" TIME = " +
		std::to_string(timeGetTime());

	size_t len = text.size();

	for (DWORD i = 0; i < g_PageSize; i++)
	{
		pagePtr[i] = text[i % len];
	}
}

int main()
{
	myPid = GetCurrentProcessId();
	std::string logName = "C:\\Users\\denis\\source\\repos\\GIT_ETU_4314\\OS\\lab\\Kuznetsov_DS\\lab_4\\temp\\Writer_" +
		std::to_string(myPid) + ".log";

	logFile.open(logName);
	if (!logFile.is_open()) std::cerr << "Cannot open writer log file\n";

	SYSTEM_INFO si; GetSystemInfo(&si);
	g_PageSize = si.dwPageSize;
	g_MapSize = sizeof(SharedMeta) + (SIZE_T)NUM_PAGES * g_PageSize;
	SetProcessWorkingSetSize(GetCurrentProcess(), 256 * 1024 * 1024, 512 * 1024 * 1024);

	hFileMap = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)g_MapSize, L"Global\\RW_LAB4_MEMORY");
	if (!hFileMap) { std::cerr << "CreateFileMapping failed\n"; return 1; }

	bool firstProcess = (GetLastError() != ERROR_ALREADY_EXISTS);
	pBase = MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (!pBase) { std::cerr << "MapViewOfFile failed\n"; Cleanup(); return 1; }

	VirtualLock(pBase, g_MapSize);
	pMeta = (SharedMeta*)pBase;
	pPages = (char*)pBase + sizeof(SharedMeta);

	if (firstProcess) {
		ZeroMemory(pMeta, sizeof(SharedMeta));
		for (int i = 0; i < NUM_PAGES; i++) {
			pMeta->readerCount[i] = 0;
			pMeta->hasData[i] = FALSE;
		}
	}

	for (int i = 0; i < NUM_PAGES; i++) {
		std::wstring name;
		name = L"Global\\RW_MUTEX_" + std::to_wstring(i);
		hMutexState[i] = firstProcess ? CreateMutexW(NULL, FALSE, name.c_str())
			: OpenMutexW(MUTEX_ALL_ACCESS, FALSE, name.c_str());

		name = L"Global\\RW_CAN_WRITE_" + std::to_wstring(i);
		hCanWrite[i] = firstProcess ? CreateSemaphoreW(NULL, 1, 1, name.c_str())
			: OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, FALSE, name.c_str());

		name = L"Global\\RW_CAN_READ_" + std::to_wstring(i);
		hCanRead[i] = firstProcess ? CreateSemaphoreW(NULL, 0, MAX_CONCURRENT_READERS, name.c_str())
			: OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, FALSE, name.c_str());

		if (!hMutexState[i] || !hCanWrite[i] || !hCanRead[i]) { i--; Sleep(10); }
	}

	srand((unsigned)time(NULL) + myPid);
	Log("START");

	int pageIdx = myPid % NUM_PAGES;

	while (true)
	{

		bool allDone = true;

		for (int i = 0; i < NUM_PAGES; i++)
		{
			if (pMeta->writeCount[i] < MAX_WRITES_PER_PAGE)
			{
				allDone = false;
				break;
			}
		}

		if (allDone)
		{
			Log("FINISHED");

			if (InterlockedCompareExchange(
				(LONG*)&pMeta->writersFinished,
				TRUE,
				FALSE) == FALSE)
			{
				for (int i = 0; i < NUM_PAGES; i++)
					ReleaseSemaphore(
						hCanRead[i],
						MAX_CONCURRENT_READERS,
						NULL);
			}

			break;
		}

		bool found = false;
		for (int i = 0; i < NUM_PAGES; i++)
		{
			if (pMeta->writeCount[i] >= MAX_WRITES_PER_PAGE)
				continue;

			if (WaitForSingleObject(hCanWrite[i], 0) == WAIT_OBJECT_0)
			{
				WaitForSingleObject(hMutexState[i], INFINITE);

				if (pMeta->writeCount[i] >= MAX_WRITES_PER_PAGE ||
					pMeta->hasData[i])
				{
					ReleaseMutex(hMutexState[i]);
					ReleaseSemaphore(hCanWrite[i], 1, NULL);
					continue;
				}

				found = true;
				pageIdx = i;

				Log("WRITING", pageIdx);

				DWORD duration = 500 + rand() % 1001;
				Sleep(duration);

				WritePage(pageIdx);

				pMeta->writeCount[pageIdx]++;
				pMeta->readerCount[pageIdx] = 0;
				pMeta->hasData[pageIdx] = TRUE;

				ReleaseMutex(hMutexState[pageIdx]);

				// разрешаем чтение
				ReleaseSemaphore(hCanRead[pageIdx], 1, NULL);

				Log("WRITE_DONE", pageIdx);

				break;
			}
		}
		if (!found)
		{
			Sleep(10);
		}


	}

	Cleanup();
	return 0;
}