#include <dirent.h> 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "comandos.h"
#include "socket.h"

char changeDirectoryServer[100] = "cd";
char listCurrentDirectoryServer[100] = "ls";
char showFileContent[100] = "ver";
char showSpecificLine[100] = "linha";
char showLinesBetween[100] = "linhas";
char editLine[100] = "edit";

char *getNewContent() {
    int line;
    char file[100];
    char content[100];
    
    scanf("%d", &line);
    scanf("%s", file);
    fgets(content, 100, stdin);

    char *newContent = cleanNewContent(content);
    return newContent;
}

char *cleanNewContent(char *newContent){
    char *ptemp = newContent;
    int count = 0;
    for(int i = 0; i <= strlen(newContent); i++){
        if(newContent[i] == '"') {
            if(count == 0) {
                count++;
                ptemp += i + 1;
            } else if(count == 1) {
                ptemp[i - 2] = 0;
            }
        }
    }
    memcpy(newContent, ptemp, strlen(newContent));
    strcat(newContent, "\n");
    return newContent;
}

int main() {
    int socket = ConexaoRawSocket("lo");
    char *command = rcv(socket, command, 100, 0);
    while (1) {
        char directoryPath[100];
        printf("%s ", getcwd(directoryPath, 100));
        //recebe mensagem

        char command[100];
        scanf("%s", command);
        //descodifica

        //verifica qual comando
        //executa comando
        if(!strcmp(command, changeDirectoryServer)) {
            char directory[100];
            scanf("%s", directory);
            changeDirectory(directory);
        }

        if(!strcmp(command, listCurrentDirectoryServer)) {
            listCurrentDirectoryFiles();
        }

        if(!strcmp(command, showFileContent)) {
            char file[100];
            scanf("%s", file);
            showFileContentServer(file);
        }

        if(!strcmp(command, showSpecificLine)) {
            int line;
            char file[100];
            scanf("%d", &line);
            scanf("%s", file);

            showSpecificLineContentServer(file, line);
        }

        if(!strcmp(command, showLinesBetween)) {
            int initialLine;
            int finalLine;
            char file[100];
            scanf("%d", &initialLine);
            scanf("%d", &finalLine);
            scanf("%s", file);

            showLinesContentInRangeServer(file, initialLine, finalLine);
        }

        if(!strcmp(command, editLine)) {
            int line;
            char file[100];
            char content[100];
            
            scanf("%d", &line);
            scanf("%s", file);
            fgets(content, 100, stdin);
            
            char *newContent = cleanNewContent(content);

            editSpecificLineContent(file, line, newContent);
        }
    
        //codifica resultado
        //envia resultado    
    }

    return(0);
}