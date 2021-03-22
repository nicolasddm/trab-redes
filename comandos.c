#include <dirent.h> 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 1000

char listCurrentDirectoryFiles(char *directoryFilesList) {
    DIR *directory;
    struct dirent *dir;
    directory = opendir(".");
    
    if (directory) {
        while ((dir = readdir(directory)) != NULL) {
            strcat(directoryFilesList, dir->d_name);
            strcat(directoryFilesList, "\n");
        }
        closedir(directory);
    }

    return *directoryFilesList;
}

int changeDirectory(char *directory) {
    return chdir(directory); 
}

void showFileContentServer(char *file, char *fileContent) {
    FILE * fPtr;

    fPtr = fopen(file, "r");
    char buffer[BUFFER_SIZE];
    int totalRead = 0;

    if(fPtr == NULL) {
        printf("Unable to open file.\n");
        printf("Please check whether file exists and you have read privilege.\n");
        exit(EXIT_FAILURE);
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
}

void showSpecificLineContentServer(char *file, int line, char *fileContent) {
    FILE * fPtr;

    fPtr = fopen(file, "r");
    char buffer[BUFFER_SIZE];
    int totalRead = 0;

    if(fPtr == NULL)
    {
        printf("Unable to open file.\n");
        printf("Please check whether file exists and you have read privilege.\n");
        exit(EXIT_FAILURE);
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
}

void showLinesContentInRangeServer(char *file, int initialLine, int finalLine,  char *fileContent) {
    FILE * fPtr;

    fPtr = fopen(file, "r");
    char buffer[BUFFER_SIZE];
    int totalRead = 0;

    if(fPtr == NULL)
    {
        printf("Unable to open file.\n");
        printf("Please check whether file exists and you have read privilege.\n");
        exit(EXIT_FAILURE);
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
}

void editSpecificLineContent(char *file, int line, char *content) {
    FILE * fPtr;
    FILE * fTemp;

    char buffer[BUFFER_SIZE];
    int count;

    fPtr  = fopen(file, "r");
    fTemp = fopen("replace.tmp", "w"); 

    if (fPtr == NULL || fTemp == NULL) {
        printf("\nUnable to open file.\n");
        printf("Please check whether file exists and you have read/write privilege.\n");
        exit(EXIT_SUCCESS);
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
}