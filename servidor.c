#include <dirent.h> 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
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

typedef struct {
    unsigned char command[100];
} cliente_t;

typedef struct {
    unsigned int start_marker: 8;
    unsigned int size: 4;
    unsigned int sequence: 4;
    unsigned int destination_address: 2;
    unsigned int source_address: 2;
    unsigned char data[15];
    unsigned int type: 4;
    unsigned int parity: 8;
} kermit_protocol_t;

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

int getMessageFromAnotherProcess(int socket, kermit_protocol_t *received_buffer)
{
    int received_code;
    received_code = recv(
        socket, 
        received_buffer,
        sizeof(kermit_protocol_t), 
        0
    );

    return received_code;

}

cliente_t *command;
int receive;
char device[] = "lo";
kermit_protocol_t received_buffer;

int main() {
    int socket = ConexaoRawSocket(device);
    printf("%d\n", socket);
    while (1) {
        char directoryPath[100];
        // printf("%s\n", getcwd(directoryPath, 100));
        //recebe mensagem

        receive = getMessageFromAnotherProcess(socket, &received_buffer);
        printf("%d\n", receive);

        if (received_buffer.type == 0b1) {
            printf("%s\n", received_buffer.data);  
        }

        // char command[100];
        // scanf("%s", command);
        //descodifica

        //verifica qual comando
        //executa comando
        // if(!strcmp(command, changeDirectoryServer)) {
        //     char directory[100];
        //     scanf("%s", directory);
        //     changeDirectory(directory);
        // }

        // if(!strcmp(command, listCurrentDirectoryServer)) {
        //     listCurrentDirectoryFiles();
        // }

        // if(!strcmp(command, showFileContent)) {
        //     char file[100];
        //     scanf("%s", file);
        //     showFileContentServer(file);
        // }

        // if(!strcmp(command, showSpecificLine)) {
        //     int line;
        //     char file[100];
        //     scanf("%d", &line);
        //     scanf("%s", file);

        //     showSpecificLineContentServer(file, line);
        // }

        // if(!strcmp(command, showLinesBetween)) {
        //     int initialLine;
        //     int finalLine;
        //     char file[100];
        //     scanf("%d", &initialLine);
        //     scanf("%d", &finalLine);
        //     scanf("%s", file);

        //     showLinesContentInRangeServer(file, initialLine, finalLine);
        // }

        // if(!strcmp(command, editLine)) {
        //     int line;
        //     char file[100];
        //     char content[100];
            
        //     scanf("%d", &line);
        //     scanf("%s", file);
        //     fgets(content, 100, stdin);
            
        //     char *newContent = cleanNewContent(content);

        //     editSpecificLineContent(file, line, newContent);
        // }
    
        //codifica resultado
        //envia resultado    
    }

    return(0);
}