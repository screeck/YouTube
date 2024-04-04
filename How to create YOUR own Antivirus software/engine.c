#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dirent.h"
#include <yara.h>
#include <Windows.h>

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

int scanCallback(
    YR_SCAN_CONTEXT* context,
    int message,
    void* message_data,
    void* user_data) {
    switch (message) {
    case CALLBACK_MSG_RULE_MATCHING:
        printf("Matched rule: %s\n", ((YR_RULE*)message_data)->identifier);
        break;
    case CALLBACK_MSG_RULE_NOT_MATCHING:
        printf("Did not match rule: %s\n", ((YR_RULE*)message_data)->identifier);
        break;
    case CALLBACK_MSG_SCAN_FINISHED:
        printf("Scan finished\n");
        break;
    case CALLBACK_MSG_TOO_MANY_MATCHES:
        printf("Too many matches\n");
        break;
    case CALLBACK_MSG_CONSOLE_LOG:
        printf("Console log: %s\n", (char*)message_data);
        break;
    default:
        break;
    }

    return CALLBACK_CONTINUE;
}


void scanFile(const char* filePath, YR_RULES* rules) {

    yr_rules_scan_file(rules, filePath, SCAN_FLAGS_REPORT_RULES_MATCHING, scanCallback, NULL, NULL);

}

void scanDirectory(const char* dirPath, YR_RULES* rules) {
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
            scanDirectory(path, rules);
        }
        else {
            char filePath[BUFFER_SIZE];
            snprintf(filePath, sizeof(filePath), "%s%c%s", dirPath, PATH_SEPARATOR, entry->d_name);
            scanFile(filePath, rules);
        }
    }
    closedir(dir);
}

void checkType(const char* path, const char* rules) {
    struct stat path_stat;
    if (stat(path, &path_stat) == 0) {
        if (S_ISREG(path_stat.st_mode)) {
            // Path is a regular file
            scanFile(path, rules);
        }
        else if (S_ISDIR(path_stat.st_mode)) {
            // Path is a directory
            scanDirectory(path, rules);
        }
        else {
            printf("Unknown file type\n");
        }
    }
    else {
        perror("Error getting file status");
    }
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("[-] Incorrect parameters specified\n");
        return 1;
    }

    const char directory_path[] = "C:/Users/mjank/Documents/MalDev/Antivirus/rules";
    char* file_path = argv[1];

    //debugger settings:
    //const char file_path[] = "dir_scan";
    //const char directory_to_scan[] = "C:/Users/mjank/Desktop/";
    // Initialize YARA

    int Initresult = yr_initialize();
    if (Initresult != 0) {
        printf("[-] Failed to initialize YARA\n");
        return 1;
    }

    printf("[+] Succesfully initialized YARA\n");


    YR_COMPILER* compiler = NULL;
    int Compilerresult = yr_compiler_create(&compiler);
    if (Compilerresult != ERROR_SUCCESS) {
        printf("[-] Failed to initialize YARA\n");
        return 1;
    }

    printf("[+] Succesfully created compiler\n");


    // Load YARA rules from each file in the directory
    DIR* directory = opendir(directory_path);
    if (directory == NULL) {
        printf("[-] Failed to open directory: %s\n", directory_path);
        yr_finalize();
        return 1;
    }

    printf("[+] Succesfully opened rules directory\n");

    struct dirent* entry;
    while ((entry = readdir(directory)) != NULL) {
        if (entry->d_type == DT_REG && strstr(entry->d_name, ".yar") != NULL) {
            char rule_file_path[PATH_MAX];
            snprintf(rule_file_path, sizeof(rule_file_path), "%s/%s", directory_path, entry->d_name);
            FILE* rule_file = fopen(rule_file_path, "rb");
                int Addresult = yr_compiler_add_file(compiler, rule_file, NULL, NULL); 
                if (Addresult > 0) {
                    printf("[-] Failed to compile YARA rule %s, number of errors found: %d\n", rule_file_path, Addresult);
                    displayErrorMessage(GetLastError());
                }
                else {
                    printf("[+] Compiled rules %s\n", rule_file_path);
                }
               fclose(rule_file);
        }         
  }
  
    closedir(directory);


    YR_RULES* rules = NULL;
    yr_compiler_get_rules(compiler, &rules);

    checkType(file_path, rules);


    // Clean up
    yr_rules_destroy(rules);
    yr_finalize();

    return 0;
}
