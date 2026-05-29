#include <iostream>
#include <string>
#include <windows.h>

const char* PIPE_NAME = "\\\\.\\pipe\\Lab4_2_Pipe";
const DWORD BUF_SIZE = 1024;

HANDLE hPipe = INVALID_HANDLE_VALUE;
HANDLE hEvent = NULL;

OVERLAPPED ovConnect = { 0 };
OVERLAPPED ovWrite = { 0 };

bool isConnected = false;

void ShowMenu() {
	std::cout << "\n=== SERVER MENU ===\n";
	std::cout << "1. Create named pipe & event\n";
	std::cout << "2. Wait for client connection\n";
	std::cout << "3. Send message (Async Write)\n";
	std::cout << "4. Disconnect client\n";
	std::cout << "5. Exit\n";
	std::cout << "Choice: ";
}

void Cleanup() {
	if (hPipe != INVALID_HANDLE_VALUE) {
		CloseHandle(hPipe);
		hPipe = INVALID_HANDLE_VALUE;
	}

	if (hEvent) {
		CloseHandle(hEvent);
		hEvent = NULL;
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
						std::cout << "[INFO] Pipe already created.\n";
						break;
					}

					hPipe = CreateNamedPipeA(
						PIPE_NAME,
						PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
						PIPE_TYPE_MESSAGE |
						PIPE_READMODE_MESSAGE |
						PIPE_WAIT,
						1,
						BUF_SIZE,
						BUF_SIZE,
						0,
						NULL
					);

					if (hPipe == INVALID_HANDLE_VALUE) {
						std::cout << "[ERROR] CreateNamedPipe failed: "
							<< GetLastError() << "\n";
						break;
					}

					hEvent = CreateEvent(
						NULL,
						TRUE,
						FALSE,
						NULL
					);

					if (!hEvent) {
						std::cout << "[ERROR] CreateEvent failed: "
							<< GetLastError() << "\n";

						Cleanup();
						break;
					}

					ZeroMemory(&ovConnect, sizeof(OVERLAPPED));
					ZeroMemory(&ovWrite, sizeof(OVERLAPPED));

					ovConnect.hEvent = hEvent;
					ovWrite.hEvent = hEvent;

					std::cout << "[OK] Named pipe created.\n";
					break;
				}

			case 2: {

					if (hPipe == INVALID_HANDLE_VALUE) {
						std::cout << "[ERROR] Pipe not created.\n";
						break;
					}

					if (isConnected) {
						std::cout << "[INFO] Client already connected.\n";
						break;
					}

					ResetEvent(hEvent);

					std::cout << "Waiting for client...\n";

					BOOL connected = ConnectNamedPipe(
						hPipe,
						&ovConnect
					);

					if (!connected) {

						DWORD err = GetLastError();

						if (err == ERROR_IO_PENDING) {

							WaitForSingleObject(
								hEvent,
								INFINITE
							);
						}
						else if (err == ERROR_PIPE_CONNECTED) {

							SetEvent(hEvent);
						}
						else {

							std::cout << "[ERROR] ConnectNamedPipe failed: "
								<< err << "\n";

							break;
						}
					}

					isConnected = true;

					std::cout << "[OK] Client connected.\n";
					break;
				}

			case 3: {

					if (!isConnected) {
						std::cout << "[ERROR] No client connected.\n";
						break;
					}

					std::string msg;

					std::cout << "Enter message: ";
					std::getline(std::cin, msg);

					if (msg.empty()) {
						std::cout << "[INFO] Empty message ignored.\n";
						break;
					}

					ResetEvent(hEvent);

					DWORD bytesWritten = 0;

					BOOL res = WriteFile(
						hPipe,
						msg.c_str(),
						(DWORD)msg.size(),
						&bytesWritten,
						&ovWrite
					);

					if (!res) {

						DWORD err = GetLastError();

						if (err == ERROR_IO_PENDING) {

							WaitForSingleObject(
								hEvent,
								INFINITE
							);

							if (GetOverlappedResult(
								hPipe,
								&ovWrite,
								&bytesWritten,
								FALSE
								)) {

								std::cout << "[OK] Async sent "
									<< bytesWritten
									<< " bytes.\n";
							}
							else {

								std::cout << "[ERROR] GetOverlappedResult failed: "
									<< GetLastError() << "\n";
							}
						}
						else {

							std::cout << "[ERROR] WriteFile failed: "
								<< err << "\n";
						}
					}
					else {

						std::cout << "[OK] Sent immediately "
							<< bytesWritten
							<< " bytes.\n";
					}

					break;
				}

			case 4: {

					if (!isConnected) {
						std::cout << "[INFO] No active connection.\n";
						break;
					}

					if (!DisconnectNamedPipe(hPipe)) {

						std::cout << "[ERROR] DisconnectNamedPipe failed: "
							<< GetLastError() << "\n";
					}
					else {

						isConnected = false;

						std::cout << "[OK] Client disconnected.\n";
					}

					break;
				}

			case 5: {

					Cleanup();

					std::cout << "Server terminated.\n";

					return 0;
				}

			default:
				std::cout << "Invalid menu option.\n";
		}
	}
}