#include <windows.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <PID> <DLL Path>\n", argv[0]);
        return 1;
    }

    DWORD processId = atoi(argv[1]);
    const char* dllPath = argv[2];

    // Open the target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL) {
        printf("Could not open process. Error code: %d\n", GetLastError());
        return 1;
    }

    // Allocate memory in the target process to store the DLL path
    LPVOID remoteBuf = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (remoteBuf == NULL) {
        printf("Could not allocate memory in target process. Error code: %d\n", GetLastError());
        CloseHandle(hProcess);
        return 1;
    }

    // Write the DLL path to the allocated memory in the target process
    if (!WriteProcessMemory(hProcess, remoteBuf, dllPath, strlen(dllPath) + 1, NULL)) {
        printf("Could not write to process memory. Error code: %d\n", GetLastError());
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    // Load kernel32.dll in the current process
    HMODULE hKernel32 = GetModuleHandleW(L"Kernel32");
    if (hKernel32 == NULL) {
        printf("Failed to load kernel32.dll. Error code: %d\n", GetLastError());
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    // Get the address of LoadLibraryA from kernel32.dll
    FARPROC loadLibAddr = GetProcAddress(hKernel32, "LoadLibraryA");
    if (loadLibAddr == NULL) {
        printf("Could not find LoadLibraryA function. Error code: %d\n", GetLastError());
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    // Create a remote thread in the target process to execute LoadLibraryA with the DLL path as argument
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibAddr, remoteBuf, 0, NULL);
    if (hThread == NULL) {
        printf("Could not create remote thread. Error code: %d\n", GetLastError());
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    // Wait for the remote thread to finish
    WaitForSingleObject(hThread, INFINITE);

    // Clean up resources
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    printf("DLL injected successfully.\n");
    return 0;
}
