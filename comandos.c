#include <dirent.h> 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define BUFFER_SIZE 1000

int listCurrentDirectoryFiles(char *directoryFilesList) {
    DIR *directory;
    struct dirent *dir;
    directory = opendir(".");
    
    if (directory) {
        while ((dir = readdir(directory)) != NULL) {
            strcat(directoryFilesList, dir->d_name);
            strcat(directoryFilesList, "\n");
        }
        closedir(directory);
    } else {
        return errno;
    }

    return 0;
}

int changeDirectory(char *directory) {
    int errorCode = chdir(directory);
    if (errorCode == -1) {
        return errno;
    }
    return 0;
}

int showFileContentServer(char *file, char *fileContent) {
    FILE * fPtr;

    fPtr = fopen(file, "r");
    char buffer[BUFFER_SIZE];
    int totalRead = 0;

    if(fPtr == NULL) {
        if(errno == 2){
            return 3;
        }
        return errno;
    }

    int count = 1;
    while (fgets(buffer, BUFFER_SIZE, fPtr) != NULL) {   
        totalRead = strlen(buffer);

        buffer[totalRead - 1] = buffer[totalRead - 1] == '\n' ? '\0' : buffer[totalRead - 1];

        char countChar[50];
        sprintf(countChar, "%d", count);
        strcat(fileContent, countChar);
        strcat(fileContent, " ");
        strcat(fileContent, buffer);
        strcat(fileContent, "\n");
        
        count++;
    }
    fclose(fPtr);
    return 0;
}

int showSpecificLineContentServer(char *file, int line, char *fileContent) {
    FILE * fPtr;

    fPtr = fopen(file, "r");
    char buffer[BUFFER_SIZE];
    int totalRead = 0;

    if(fPtr == NULL) {
        if(errno == 2){
            return 3;
        }
        return errno;
    }

    int count = 1;
    while (fgets(buffer, BUFFER_SIZE, fPtr) != NULL) {
        totalRead = strlen(buffer);

        buffer[totalRead - 1] = buffer[totalRead - 1] == '\n' ? '\0' : buffer[totalRead - 1];
        if (count == line) {
            char countChar[50];
            sprintf(countChar, "%d", count);
            strcat(fileContent, countChar);
            strcat(fileContent, " ");
            strcat(fileContent, buffer);
            strcat(fileContent, "\n");
        }
        count++;
    }
    fclose(fPtr);
    if(line > count) {
        return 4;
    }
    return 0;
}

int showLinesContentInRangeServer(char *file, int initialLine, int finalLine,  char *fileContent) {
    FILE * fPtr;

    fPtr = fopen(file, "r");
    char buffer[BUFFER_SIZE];
    int totalRead = 0;

    if(fPtr == NULL){
        if(errno == 2){
            return 3;
        }
        return errno;
    }

    int count = 1;
    while (fgets(buffer, BUFFER_SIZE, fPtr) != NULL) {
        totalRead = strlen(buffer);

        buffer[totalRead - 1] = buffer[totalRead - 1] == '\n' ? '\0' : buffer[totalRead - 1];
        if ((initialLine <= count) && (count <= finalLine)) {
            char countChar[50];
            sprintf(countChar, "%d", count);
            strcat(fileContent, countChar);
            strcat(fileContent, " ");
            strcat(fileContent, buffer);
            strcat(fileContent, "\n");
        }
        count++;
    }
    
    fclose(fPtr);
    if(initialLine > count) {
        return 4;
    }
    return 0;
}

int editSpecificLineContent(char *file, int line, char *content) {
    FILE * fPtr;
    FILE * fTemp;

    char buffer[BUFFER_SIZE];
    int count;

    fPtr  = fopen(file, "r");
    fTemp = fopen("replace.tmp", "w"); 

    if (fPtr == NULL || fTemp == NULL) {
        if(errno == 2){
            return 3;
        }
        return errno;
    }

    count = 0;
    while ((fgets(buffer, BUFFER_SIZE, fPtr)) != NULL) {
        count++;

        if (count == line)
            fputs(content, fTemp);
        else
            fputs(buffer, fTemp);
    }
    fclose(fPtr);
    fclose(fTemp);

    remove(file);
    rename("replace.tmp", file);

    if(line > count) {
        return 4;
    }

    return 0;
}
