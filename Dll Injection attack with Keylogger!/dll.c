#include <windows.h>

// Function inside the DLL
__declspec(dllexport) void sayHello() {
    MessageBoxA(NULL, "Hello from the DLL!", "DLL Message", MB_OK);
}

// Entry point of the DLL
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            // Code to run when the DLL is loaded
            sayHello(); // Call the function when the DLL is loaded
            break;

        case DLL_PROCESS_DETACH:
            // Code to run when the DLL is unloaded
            break;

        case DLL_THREAD_ATTACH:
            // Code to run when a thread is created
            break;

        case DLL_THREAD_DETACH:
            // Code to run when a thread ends
            break;
    }
    return TRUE;
}
