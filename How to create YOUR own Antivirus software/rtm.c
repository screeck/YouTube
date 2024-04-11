#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <Windows.h>
#include <string.h>
#include <stdio.h>

void displayErrorMessage(DWORD errorCode) {
	LPSTR messageBuffer = NULL;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&messageBuffer,
		0,
		NULL);

	if (messageBuffer != NULL) {
		printf("Error: %s\n", messageBuffer);
		LocalFree(messageBuffer);
	}
	else {
		printf("Error: Unable to get error message for code %d\n", errorCode);
	}
}

void CallDetectionEngine(const char* filePath) {
	printf("[+] Called detection\n");
	fflush(stdout);

	// Construct the command to execute, including the file path and the parameter
	char command[1024];
	sprintf(command, "C:/Users/mjank/Documents/MalDev/Antivirus/engine/x64/Debug/engine.exe \"%s\"", filePath);

	// Execute the command using system()
	int returnCode = system(command);

	if (returnCode != 0) {
		fprintf(stderr, "[-] Failed to execute command.\n");
	}
}


DWORD MonitorDirectoryThread(LPCSTR directoryPath) {

	HANDLE hDir = CreateFileA(directoryPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

	if (hDir == INVALID_HANDLE_VALUE) {
		printf("[-] Failed to open directory: %s\n", directoryPath);
		//displayErrorMessage(GetLastError()); // Assuming you have a function to display error message
		return 1;
	}


	char buffer[4096];
	DWORD bytesReturned;
	BOOL result;

	while (TRUE) {
		result = ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
			&bytesReturned, NULL, NULL);

		if (!result) {
			printf("[-] Error reading directory changes for: %s with error code %d\n", directoryPath, GetLastError());
			break;
		}

		//printf("[+] Change detected in directory: %s\n", directoryPath);
		//fflush(stdout);
		// Process buffer to get file path
		FILE_NOTIFY_INFORMATION* pFileNotifyInfo = (FILE_NOTIFY_INFORMATION*)buffer;
		while (pFileNotifyInfo) {
			// Convert wide char to multibyte string
			WCHAR fileName[MAX_PATH];
			memcpy(fileName, pFileNotifyInfo->FileName, pFileNotifyInfo->FileNameLength);
			fileName[pFileNotifyInfo->FileNameLength / sizeof(WCHAR)] = L'\0';
			char mbFileName[MAX_PATH];
			wcstombs(mbFileName, fileName, sizeof(mbFileName));

			// Construct full path
			char fullPath[MAX_PATH];
			snprintf(fullPath, MAX_PATH, "%s\\%s", directoryPath, mbFileName);

			// Print full path
			printf("[+] Change detected in file: %s\n", fullPath);
			fflush(stdout);
			// Call your detection engine with the file path
			CallDetectionEngine(fullPath);

			// Move to next FILE_NOTIFY_INFORMATION structure
			if (pFileNotifyInfo->NextEntryOffset == 0)
				break;
			pFileNotifyInfo = (FILE_NOTIFY_INFORMATION*)((LPBYTE)pFileNotifyInfo + pFileNotifyInfo->NextEntryOffset);
		}

		//ReadDirectoryChangesW will overwrite the buffer with new change notifications on each call. NO NEED TO CLEAR THE BUFFER
	}

	return 0;
}


int main(int argc, char* argv[]) {

	//if (argc != 2) {
	//	printf("[-] error while reading arguments");
	//	return 1;
	//}

	//unsigned char *pathList = argv[1];

	//for debugging
	unsigned char pathList[] = "C:\\Users\\mjank\\Desktop;";


	char *token;
	token = strtok(pathList, ";");

	while (token != NULL) {
		HANDLE hThread = CreateThread(NULL, 0, MonitorDirectoryThread, token, 0, NULL);
		if (hThread == NULL) {
			printf("Failed to create thread for directory monitoring\n");
			return 1;
		};
		token = strtok(NULL, ";");
	}

	



	//printf("Press 'q' followed by Enter to exit...\n");

	// Wait for user input to exit
	char userInput;
	do {
		userInput = getchar();
	} while (userInput != 'q');


	return 0;
}