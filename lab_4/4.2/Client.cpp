#include <cstring>
#include <iostream>
#include <windows.h>

const char* PIPE_NAME = "\\\\.\\pipe\\Lab4_2_Pipe";
const DWORD BUF_SIZE = 1024;

HANDLE hPipe = INVALID_HANDLE_VALUE;

char recvBuf[BUF_SIZE] = { 0 };

bool dataReceived = false;

OVERLAPPED ovRead = { 0 };

VOID WINAPI ReadCompletion(
	DWORD dwErrorCode,
	DWORD dwBytesTransferred,
	LPOVERLAPPED lpOverlapped
) {

	if (dwErrorCode == 0) {

		recvBuf[dwBytesTransferred] = '\0';

		std::cout << "\n[RECEIVED] "
			<< recvBuf
			<< "\n";
	}
	else {

		std::cout << "\n[ERROR] Read failed: "
			<< dwErrorCode
			<< "\n";
	}

	dataReceived = true;
}

void ShowMenu() {

	std::cout << "\n=== CLIENT MENU ===\n";
	std::cout << "1. Connect to server\n";
	std::cout << "2. Read message (Async Read)\n";
	std::cout << "3. Disconnect\n";
	std::cout << "4. Exit\n";
	std::cout << "Choice: ";
}

void DisconnectClient() {

	if (hPipe != INVALID_HANDLE_VALUE) {

		CloseHandle(hPipe);

		hPipe = INVALID_HANDLE_VALUE;

		std::cout << "[OK] Disconnected.\n";
	}
	else {

		std::cout << "[INFO] Not connected.\n";
	}
}

int main() {

	int choice = 0;

	while (true) {

		ShowMenu();

		std::cin >> choice;
		std::cin.ignore();

		switch (choice) {

			case 1: {

					if (hPipe != INVALID_HANDLE_VALUE) {

						std::cout << "[INFO] Already connected.\n";
						break;
					}

					hPipe = CreateFileA(
						PIPE_NAME,
						GENERIC_READ | GENERIC_WRITE,
						0,
						NULL,
						OPEN_EXISTING,
						FILE_FLAG_OVERLAPPED,
						NULL
					);

					if (hPipe == INVALID_HANDLE_VALUE) {

						std::cout << "[ERROR] CreateFile failed: "
							<< GetLastError()
							<< "\n";

						break;
					}

					ZeroMemory(&ovRead, sizeof(OVERLAPPED));

					std::cout << "[OK] Connected to server.\n";

					break;
				}

			case 2: {

					if (hPipe == INVALID_HANDLE_VALUE) {

						std::cout << "[ERROR] Not connected.\n";
						break;
					}

					std::memset(recvBuf, 0, BUF_SIZE);

					dataReceived = false;

					std::cout << "Waiting for server message...\n";

					BOOL res = ReadFileEx(
						hPipe,
						recvBuf,
						BUF_SIZE - 1,
						&ovRead,
						ReadCompletion
					);

					if (!res) {

						std::cout << "[ERROR] ReadFileEx failed: "
							<< GetLastError()
							<< "\n";

						break;
					}

					while (!dataReceived) {

						SleepEx(50, TRUE);
					}

					break;
				}

			case 3: {

					DisconnectClient();
					break;
				}

			case 4: {

					DisconnectClient();

					std::cout << "Client terminated.\n";

					return 0;
				}

			default:
				std::cout << "Invalid menu option.\n";
		}
	}
}