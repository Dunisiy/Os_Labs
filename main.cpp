#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

void ListDrives() {
	DWORD drives = GetLogicalDrives();
	std::cout << "Available drives:" << std::endl;
	for (char letter = 'A'; letter <= 'Z'; ++letter) {
		if (drives & 1) {
			std::cout << letter << ": " << std::endl;
		}
		drives >>= 1;
	}

	wchar_t driveStrings[256];
	GetLogicalDriveStringsW(256, driveStrings);
	std::wcout << L"Drive strings: " << driveStrings << std::endl;
}

void GetDriveInfo(const std::string& drive) {
	std::wstring wdrive(drive.begin(), drive.end());

	UINT type = GetDriveTypeW(wdrive.c_str());
	std::cout << "Drive type: ";
	switch (type) {
		case DRIVE_UNKNOWN: std::cout << "Unknown"; break;
		case DRIVE_NO_ROOT_DIR: std::cout << "No root dir"; break;
		case DRIVE_REMOVABLE: std::cout << "Removable"; break;
		case DRIVE_FIXED: std::cout << "Fixed"; break;
		case DRIVE_REMOTE: std::cout << "Remote"; break;
		case DRIVE_CDROM: std::cout << "CD-ROM"; break;
		case DRIVE_RAMDISK: std::cout << "RAM disk"; break;
	}
	std::cout << std::endl;

	wchar_t volumeName[256];
	DWORD serialNumber, maxComponentLen, fileSysFlags;
	wchar_t fileSysName[256];

	if (GetVolumeInformationW(
		wdrive.c_str(),
		volumeName, 256,
		&serialNumber,
		&maxComponentLen,
		&fileSysFlags,
		fileSysName, 256))
	{
		std::wcout << L"Volume name: " << volumeName << std::endl;
		std::cout << "Serial number: " << serialNumber << std::endl;
		std::wcout << L"File system: " << fileSysName << std::endl;

		std::cout << "Max filename length: " << maxComponentLen << std::endl;

		std::cout << "File system flags:" << std::endl;

		if (fileSysFlags & FILE_CASE_SENSITIVE_SEARCH)
			std::cout << "  - Case sensitive search" << std::endl;

		if (fileSysFlags & FILE_CASE_PRESERVED_NAMES)
			std::cout << "  - Case preserved names" << std::endl;

		if (fileSysFlags & FILE_UNICODE_ON_DISK)
			std::cout << "  - Unicode on disk" << std::endl;

		if (fileSysFlags & FILE_PERSISTENT_ACLS)
			std::cout << "  - Supports ACLs" << std::endl;

		if (fileSysFlags & FILE_FILE_COMPRESSION)
			std::cout << "  - File compression supported" << std::endl;

		if (fileSysFlags & FILE_VOLUME_QUOTAS)
			std::cout << "  - Disk quotas supported" << std::endl;

		if (fileSysFlags & FILE_SUPPORTS_SPARSE_FILES)
			std::cout << "  - Sparse files supported" << std::endl;

		if (fileSysFlags & FILE_SUPPORTS_REPARSE_POINTS)
			std::cout << "  - Reparse points supported" << std::endl;

		if (fileSysFlags & FILE_SUPPORTS_REMOTE_STORAGE)
			std::cout << "  - Remote storage supported" << std::endl;

		if (fileSysFlags & FILE_VOLUME_IS_COMPRESSED)
			std::cout << "  - Volume is compressed" << std::endl;

		if (fileSysFlags & FILE_SUPPORTS_OBJECT_IDS)
			std::cout << "  - Object IDs supported" << std::endl;

		if (fileSysFlags & FILE_SUPPORTS_ENCRYPTION)
			std::cout << "  - Encryption supported" << std::endl;

		if (fileSysFlags & FILE_NAMED_STREAMS)
			std::cout << "  - Named streams supported" << std::endl;

		if (fileSysFlags & FILE_READ_ONLY_VOLUME)
			std::cout << "  - Read-only volume" << std::endl;

		if (fileSysFlags & FILE_SEQUENTIAL_WRITE_ONCE)
			std::cout << "  - Sequential write once" << std::endl;

		if (fileSysFlags & FILE_SUPPORTS_TRANSACTIONS)
			std::cout << "  - Transactions supported" << std::endl;
	}
	else {
		std::cout << "Failed to get volume information. Error: " << GetLastError() << std::endl;
	}

	ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
	if (GetDiskFreeSpaceExW(wdrive.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
		std::cout << "Free space: " << freeBytesAvailable.QuadPart / (1024 * 1024) << " MB" << std::endl;
		std::cout << "Total space: " << totalBytes.QuadPart / (1024 * 1024) << " MB" << std::endl;
	}
	else {
		std::cout << "Failed to get disk free space. Error: " << GetLastError() << std::endl;
	}
}

void CreateDirectoryFunc(const std::string& path) {
	std::wstring wpath(path.begin(), path.end());
	if (CreateDirectoryW(wpath.c_str(), NULL)) {
		std::cout << "Directory created successfully." << std::endl;
	}
	else {
		std::cout << "Failed to create directory. Error: " << GetLastError() << std::endl;
	}
}

void RemoveDirectoryFunc(const std::string& path) {
	std::wstring wpath(path.begin(), path.end());
	if (RemoveDirectoryW(wpath.c_str())) {
		std::cout << "Directory removed successfully." << std::endl;
	}
	else {
		std::cout << "Failed to remove directory. Error: " << GetLastError() << std::endl;
	}
}

void CreateFileFunc(const std::string& path) {
	std::wstring wpath(path.begin(), path.end());
	HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		std::cout << "File created successfully." << std::endl;
		CloseHandle(hFile);
	}
	else {
		std::cout << "Failed to create file. Error: " << GetLastError() << std::endl;
	}
}

void CopyFileFunc(const std::string& src, const std::string& dest) {
	std::wstring wsrc(src.begin(), src.end());
	std::wstring wdest(dest.begin(), dest.end());
	if (CopyFileW(wsrc.c_str(), wdest.c_str(), TRUE)) {  // FAIL_IF_EXISTS = TRUE
		std::cout << "File copied successfully." << std::endl;
	}
	else {
		DWORD err = GetLastError();
		if (err == ERROR_ALREADY_EXISTS) {
			std::cout << "Destination file already exists." << std::endl;
		}
		else {
			std::cout << "Failed to copy file. Error: " << err << std::endl;
		}
	}
}

void MoveFileFunc(const std::string& src, const std::string& dest) {
	std::wstring wsrc(src.begin(), src.end());
	std::wstring wdest(dest.begin(), dest.end());
	if (MoveFileExW(wsrc.c_str(), wdest.c_str(), 0)) {
		std::cout << "File moved successfully." << std::endl;
	}
	else {
		DWORD err = GetLastError();
		if (err == ERROR_ALREADY_EXISTS) {
			std::cout << "Destination file already exists." << std::endl;
		}
		else {
			std::cout << "Failed to move file. Error: " << err << std::endl;
		}
	}
}

void GetFileAttributesFunc(const std::string& path) {
	std::wstring wpath(path.begin(), path.end());
	DWORD attrs = GetFileAttributesW(wpath.c_str());
	if (attrs != INVALID_FILE_ATTRIBUTES) {
		std::cout << "File attributes: " << std::hex << attrs << std::endl;
		if (attrs & FILE_ATTRIBUTE_READONLY) std::cout << "Read-only" << std::endl;
		if (attrs & FILE_ATTRIBUTE_HIDDEN) std::cout << "Hidden" << std::endl;
		if (attrs & FILE_ATTRIBUTE_SYSTEM) std::cout << "System" << std::endl;
		if (attrs & FILE_ATTRIBUTE_DIRECTORY) std::cout << "Directory" << std::endl;
		if (attrs & FILE_ATTRIBUTE_ARCHIVE) std::cout << "Archive" << std::endl;
		if (attrs & FILE_ATTRIBUTE_NORMAL) std::cout << "Normal" << std::endl;
		if (attrs & FILE_ATTRIBUTE_TEMPORARY) std::cout << "Temporary" << std::endl;
	}
	else {
		std::cout << "Failed to get attributes. Error: " << GetLastError() << std::endl;
	}

	HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		BY_HANDLE_FILE_INFORMATION info;
		if (GetFileInformationByHandle(hFile, &info)) {
			std::cout << "File size: " << info.nFileSizeHigh * (MAXDWORD + 1) + info.nFileSizeLow << " bytes" << std::endl;
			std::cout << "Number of links: " << info.nNumberOfLinks << std::endl;
		}

		FILETIME creationTime, lastAccessTime, lastWriteTime;
		if (GetFileTime(hFile, &creationTime, &lastAccessTime, &lastWriteTime)) {
			SYSTEMTIME st;
			FileTimeToSystemTime(&creationTime, &st);
			std::cout << std::dec << "Creation time: " << st.wYear << "-" << st.wMonth << "-" << st.wDay << " " << st.wHour << ":" << st.wMinute << ":" << st.wSecond << std::endl;

			FileTimeToSystemTime(&lastAccessTime, &st);
			std::cout << std::dec << "Last access time: " << st.wYear << "-" << st.wMonth << "-" << st.wDay << " " << st.wHour << ":" << st.wMinute << ":" << st.wSecond << std::endl;

			FileTimeToSystemTime(&lastWriteTime, &st);
			std::cout << std::dec << "Last write time: " << st.wYear << "-" << st.wMonth << "-" << st.wDay << " " << st.wHour << ":" << st.wMinute << ":" << st.wSecond << std::endl;
		}
		CloseHandle(hFile);
	}
	else {
		std::cout << "Failed to open file for info. Error: " << GetLastError() << std::endl;
	}
}

void SetFileAttributesFunc(const std::string& path, DWORD newAttrs) {
	std::wstring wpath(path.begin(), path.end());
	if (SetFileAttributesW(wpath.c_str(), newAttrs)) {
		std::cout << "Attributes set successfully." << std::endl;
	}
	else {
		std::cout << "Failed to set attributes. Error: " << GetLastError() << std::endl;
	}
}

void SetFileTimeFunc(const std::string& path) {
	std::wstring wpath(path.begin(), path.end());
	HANDLE hFile = CreateFileW(wpath.c_str(), FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		FILETIME ft;
		SYSTEMTIME st;
		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &ft);

		if (SetFileTime(hFile, &ft, &ft, &ft)) {
			std::cout << "File times set to current time successfully." << std::endl;
		}
		else {
			std::cout << "Failed to set file times. Error: " << GetLastError() << std::endl;
		}
		CloseHandle(hFile);
	}
	else {
		std::cout << "Failed to open file for setting time. Error: " << GetLastError() << std::endl;
	}
}

int main() {
	char choice;
	std::string input1, input2;
	DWORD attrs;

	while (true) {
		std::cout << "\nMenu:\n";
		std::cout << "1. List drives\n";
		std::cout << "2. Get drive info\n";
		std::cout << "3. Create directory\n";
		std::cout << "4. Remove directory\n";
		std::cout << "5. Create file\n";
		std::cout << "6. Copy file\n";
		std::cout << "7. Move file\n";
		std::cout << "8. Get file attributes and info\n";
		std::cout << "9. Set file attributes\n";
		std::cout << "0. Set file time\n";
		std::cout << "e. Exit\n";
		std::cout << "Enter choice: ";
		std::cin >> choice;
		std::cin.clear();
		std::cin.ignore(100, '\n');

		switch (choice) {
			case '1':
				ListDrives();
				break;
			case '2':
				std::cout << "Enter drive (e.g., C:\\): ";
				std::getline(std::cin, input1);
				GetDriveInfo(input1);
				break;
			case '3':
				std::cout << "Enter directory path: ";
				std::getline(std::cin, input1);
				CreateDirectoryFunc(input1);
				break;
			case '4':
				std::cout << "Enter directory path: ";
				std::getline(std::cin, input1);
				RemoveDirectoryFunc(input1);
				break;
			case '5':
				std::cout << "Enter file path: ";
				std::getline(std::cin, input1);
				CreateFileFunc(input1);
				break;
			case '6':
				std::cout << "Enter source file: ";
				std::getline(std::cin, input1);
				std::cout << "Enter destination file: ";
				std::getline(std::cin, input2);
				CopyFileFunc(input1, input2);
				break;
			case '7':
				std::cout << "Enter source file: ";
				std::getline(std::cin, input1);
				std::cout << "Enter destination file: ";
				std::getline(std::cin, input2);
				MoveFileFunc(input1, input2);
				break;
			case '8':
				std::cout << "Enter file path: ";
				std::getline(std::cin, input1);
				GetFileAttributesFunc(input1);
				break;
			case '9':
				std::cout << "Enter file path: ";
				std::getline(std::cin, input1);
				std::cout << "Enter new attributes (hex): ";
				std::cin >> std::hex >> attrs;
				std::cin.ignore();
				SetFileAttributesFunc(input1, attrs);
				break;
			case '0':
				std::cout << "Enter file path: ";
				std::getline(std::cin, input1);
				SetFileTimeFunc(input1);
				break;
			case 'e':
				return 0;
			default:
				std::cout << "Invalid choice." << std::endl;
				std::cin.clear();
				std::cin.sync();

		}
	}
	return 0;
}