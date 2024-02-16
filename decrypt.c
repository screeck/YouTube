#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include "dirent.h"
#include <time.h>
#include <stdint.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#pragma comment(lib,"Crypt32.lib")


#define BUFFER_SIZE 1024
#define PATH_SEPARATOR '\\'


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

void decryptFile(const char* filePath, const char* password) {
    HANDLE hFile = CreateFileA(filePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error opening file \n");
        return;
    }
    printf("[+] Succesfully opened the file\n");



    HCRYPTPROV hCryptProv;
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        printf("Error acquiring crypto context\n");
        CloseHandle(hFile);
        return;
    }
    printf("[+] Succesfully acquired crypto context\n");


    HCRYPTHASH hHash;
    if (!CryptCreateHash(hCryptProv, CALG_SHA_256, 0, 0, &hHash)) {
        printf("Error creating hash\n");
        CryptReleaseContext(hCryptProv, 0);
        CloseHandle(hFile);
        return;
    }
    printf("[+] Succesfully created hash: %d\n", hHash);


    if (!CryptHashData(hHash, (const BYTE*)password, strlen(password), 0)) {
        printf("Error hashing data\n");
        CryptDestroyHash(hHash);
        CryptReleaseContext(hCryptProv, 0);
        CloseHandle(hFile);
        return;
    }
    printf("[+] Succesfully hashed the key: %s\n", password);


    HCRYPTKEY hKey;
    if (!CryptDeriveKey(hCryptProv, CALG_AES_256, hHash, 0, &hKey)) {
        printf("Error deriving key\n");
        CryptDestroyHash(hHash);
        CryptReleaseContext(hCryptProv, 0);
        CloseHandle(hFile);
        return;
    }
    printf("[+] Succesfully derived AES key: %s\n", hKey);


    DWORD dwFileSize = GetFileSize(hFile, NULL);
    BYTE* pBuffer = (BYTE*)malloc(dwFileSize);
    DWORD dwBytesRead = 0;
    if (!ReadFile(hFile, pBuffer, dwFileSize, &dwBytesRead, NULL)) {
        printf("Error reading file\n");
        free(pBuffer);
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hCryptProv, 0);
        CloseHandle(hFile);
        return;
    }
    printf("[+] Succesfully saved encrypted file to a buffer: %s\n", pBuffer);

    if (!CryptDecrypt(hKey, 0, TRUE, 0, pBuffer, &dwBytesRead)) {
        printf("Error Decrypting data \n"); 
        displayErrorMessage(GetLastError());
        free(pBuffer);
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hCryptProv, 0);
        CloseHandle(hFile);
        return;
    }

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    DWORD dwBytesWritten = 0;
    if (!WriteFile(hFile, pBuffer, dwBytesRead, &dwBytesWritten, NULL)) {
        printf("Error writing decrypted data to file\n");
    }
    else {
        SetEndOfFile(hFile);
        printf("File decrypted successfully\n");
    }


    free(pBuffer);
    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hCryptProv, 0);
    CloseHandle(hFile);
}

void decryptDirectory(const char* dirPath, const char* key) {
    DIR* dir;
    struct dirent* entry;

    if (!(dir = opendir(dirPath))) {
        printf("Error opening directory\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char path[BUFFER_SIZE];
            snprintf(path, sizeof(path), "%s%c%s", dirPath, PATH_SEPARATOR, entry->d_name);
            decryptDirectory(path, key);
        }
        else {
            char filePath[BUFFER_SIZE];
            snprintf(filePath, sizeof(filePath), "%s%c%s", dirPath, PATH_SEPARATOR, entry->d_name);
            decryptFile(filePath, key);
        }
    }
    closedir(dir);
}

BYTE* decryptAES(const char* AESDONTDELEATE, const char* SPrivKey, const char* decrypted, DWORD* decryptedAESKeyLen) {

    HANDLE hPrivateKeyFile = CreateFileA(SPrivKey, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hPrivateKeyFile == INVALID_HANDLE_VALUE) {
        printf("[-] Error opening file");
        displayErrorMessage(GetLastError());
        return;
    }
    DWORD privateKeyLength = GetFileSize(hPrivateKeyFile, NULL);
    BYTE* privateKeyData = (BYTE*)malloc(privateKeyLength);
    if (privateKeyData == NULL) {
        printf("[-] Error while allocating memory for publicKeyData");
        displayErrorMessage(GetLastError());
        return;
    }
    DWORD dwPrivKeyBytesRead = 0;
    if (!ReadFile(hPrivateKeyFile, privateKeyData, privateKeyLength, &dwPrivKeyBytesRead, NULL))

        printf("[?] Public key:\n");
    for (DWORD i = 0; i < privateKeyLength; ++i) {
        printf("%c", privateKeyData[i]);
    }
    printf("\n");


    HCRYPTPROV hCryptProv;
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        printf("CryptAcquireContext failed. Error: %d\n", GetLastError());
        return NULL;
    }

    DWORD PrivateKeyBufferSize = 0;
    if (!CryptStringToBinaryA(privateKeyData, privateKeyLength, CRYPT_STRING_BASE64HEADER, NULL, &PrivateKeyBufferSize, NULL, NULL)) {
        printf("CryptStringToBinaryA 1 failed. Error: %d\n", GetLastError());
        CryptReleaseContext(hCryptProv, 0);
        return NULL;

    }

    BYTE* PrivateKeyBuffer = (BYTE*)malloc(PrivateKeyBufferSize);
    if (!CryptStringToBinaryA(privateKeyData, privateKeyLength, CRYPT_STRING_BASE64HEADER, PrivateKeyBuffer, &PrivateKeyBufferSize, NULL, NULL)) {
        printf("CryptStringToBinaryA 2 failed. Error: %d\n", GetLastError());
        CryptReleaseContext(hCryptProv, 0);
        free(PrivateKeyBuffer);
        return NULL;

    }

    DWORD derSize = 0;
    if (!CryptDecodeObjectEx(X509_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY, PrivateKeyBuffer, PrivateKeyBufferSize, 0, NULL, NULL, &derSize)) {
        printf("CryptDecodeObjectEx 1 failed. Error: %d\n", GetLastError());
        CryptReleaseContext(hCryptProv, 0);
        free(PrivateKeyBuffer);
        return NULL;

    }

    BYTE* pPrivKeyBLOB = (BYTE*)malloc(derSize);
    if (!CryptDecodeObjectEx(X509_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY, PrivateKeyBuffer, PrivateKeyBufferSize, 0, NULL, pPrivKeyBLOB, &derSize)) {
        printf("CryptDecodeObjectEx 2 failed. Error: %d\n", GetLastError());
        CryptReleaseContext(hCryptProv, 0);
        free(pPrivKeyBLOB);
        free(PrivateKeyBuffer);
        return NULL;
    }

    HCRYPTKEY hPrivateKey;
    if (!CryptImportKey(hCryptProv, pPrivKeyBLOB, PrivateKeyBufferSize, 0, CRYPT_EXPORTABLE, &hPrivateKey)) {
        printf("CryptImportKey failed. Error: %d\n", GetLastError());
        CryptReleaseContext(hCryptProv, 0);
        free(pPrivKeyBLOB);
        free(PrivateKeyBuffer);
        return NULL;
    }



    // Read the encrypted AES key from file
    HANDLE hFileAES = CreateFileA(AESDONTDELEATE, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFileAES == INVALID_HANDLE_VALUE) {
        printf("Error opening file %d\n", GetLastError());
        return;
    }

    DWORD dwFileSize = GetFileSize(hFileAES, NULL);
    BYTE* encryptedAESKey = (BYTE*)malloc(dwFileSize);
    DWORD dwBytesRead;
    if (!ReadFile(hFileAES, encryptedAESKey, dwFileSize, &dwBytesRead, NULL)) {
        printf("Error reading file %d\n", GetLastError());
        CloseHandle(hFileAES);
        return;
    }
    CloseHandle(hFileAES);

    // Decrypt the AES key using the public key
    DWORD decryptedBufferLen = dwFileSize; // Assuming decrypted buffer will be the same size as encrypted buffer
    if (!CryptDecrypt(hPrivateKey, 0, TRUE, 0, encryptedAESKey, &decryptedAESKeyLen)) { //key does not exist
        printf("Error decrypting data\n");
        displayErrorMessage(GetLastError());
        CryptDestroyKey(hPrivateKey);
        CryptReleaseContext(hCryptProv, 0);
        free(encryptedAESKey);
        return NULL;
    }
    printf("[+] Successfully decrypted the AES key\n");


    HANDLE hFile = CreateFileA(decrypted, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error opening file %d\n", GetLastError());
    }
    DWORD dwBytesWrittenAes = 0;
    if (!WriteFile(hFile, encryptedAESKey, dwFileSize, &dwBytesWrittenAes, NULL)) {
        printf("Error writing to file %d\n", GetLastError());
    }
    printf("[+] Encrypted AES key written to file\n");

    return encryptedAESKey;
}

int main() {
    const char* path = "PUT_SOMETHING_HERE";
    const char* SPrivKey = "PUT_SOMETHING_HERE";
    const char* AESDONTDELEATE = "PUT_SOMETHING_HERE";
    const char* decrypted = "PUT_SOMETHING_HERE";
    unsigned char decryptedAES[257];

    DWORD decryptedAESKeyLen;
    BYTE* functionOutput = decryptAES(AESDONTDELEATE, SPrivKey, decrypted, &decryptedAESKeyLen);

    memcpy(decryptedAES, functionOutput, 256);
    decryptedAES[256] = '\0';
    printf("Method 2: %s\n", decryptedAES);
    decryptDirectory(path, decryptedAES);
 
    return 0;
}
