#include "Windows.h"


void dont_execute() {

    MessageBox(NULL, L"No malware here :)", L"Legit app", MB_OK);

}
void execute() {

    MessageBox(NULL, L"Malicious code is running", L"Malware", MB_OK);

}


int main() {
    
    HKEY hKey;
    LONG resoult;

    const char* keyPath = "SYSTEM\\CurrentControlSet\\Services\\VBoxGuest";
    resoult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey);

    if (resoult == ERROR_SUCCESS) {
        dont_execute();
    }
    else {
        execute();
    }
   
    return 0;
}
