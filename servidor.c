#include <dirent.h> 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
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
    unsigned char data[256];
    unsigned int type: 4;
    unsigned int parity: 8;
} kermit_protocol_t;

kermit_protocol_t *defineProtocol(
    int destination_address,
    int source_address,
    int type,
    char *message,
    int sequence
){
    kermit_protocol_t *kermit = (kermit_protocol_t *)calloc(1,sizeof(kermit_protocol_t));
    
    kermit->start_marker = 0b01111110;
    kermit->destination_address = destination_address;
    kermit->source_address = source_address;
    strcpy(kermit->data, message);
    kermit->sequence = sequence;
    kermit->type = type;
    kermit->size = strlen(message);

    return kermit;
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

int getMessageFromAnotherProcess(int socket, kermit_protocol_t *received_buffer) {
    int received_code;
    received_code = recv(
        socket, 
        received_buffer,
        sizeof(kermit_protocol_t), 
        0
    );
    return received_code;
}

int serverAddress = 0b10;
int clientAddress = 0b01;
int changeDirectoryType = 0b0000;
int listCurrentDirectoryType = 0b0001;
int showFileContentType = 0b0010;
int showSpecificLineType = 0b0011;
int showLinesBetweenType = 0b0100;
int editLineType = 0b0101;
int listDirectoryContentType = 0b1011;
int fileContentType = 0b1100;
int linesType = 0b1010;
int ACKType = 0b1000;
int NACKType = 0b1000;


cliente_t *command;
int receive, received_code;
char device[] = "lo";
kermit_protocol_t received_buffer;


int main() {
    int socket = ConexaoRawSocket(device);
    char directoryPath[100];
    printf("%s\n", getcwd(directoryPath, 100));
    
    while (1) {
        
        //recebe mensagem
        receive = getMessageFromAnotherProcess(socket, &received_buffer);
        receive = getMessageFromAnotherProcess(socket, &received_buffer);

        //verifica qual comando
        //executa comando

        //todo
        if (received_buffer.type == changeDirectoryType) {
            printf("%s\n", received_buffer.data);
            changeDirectory(received_buffer.data);
            
            kermit_protocol_t *send_buffer;
            send_buffer = defineProtocol(clientAddress, serverAddress, ACKType, getcwd(directoryPath, 100), 0);
            received_code = send(socket, send_buffer, sizeof(kermit_protocol_t), 0);
            
            free(send_buffer);
        }

        //done
        if (received_buffer.type == listCurrentDirectoryType) {
            char directoryFilesList[256] = "";
            memset(directoryFilesList, 0, 256);
            listCurrentDirectoryFiles(directoryFilesList);

            kermit_protocol_t *send_buffer;
            send_buffer = defineProtocol(clientAddress, serverAddress, listDirectoryContentType, directoryFilesList, 0);
            received_code = send(socket, send_buffer, sizeof(kermit_protocol_t), 0);
            
            free(send_buffer);         
        }

        //done
        if (received_buffer.type == showFileContentType && received_buffer.source_address == clientAddress) {
            char fileContent[256] = "";
            showFileContentServer(received_buffer.data, fileContent);

            kermit_protocol_t *send_buffer;
            send_buffer = defineProtocol(clientAddress, serverAddress, fileContentType, fileContent, 0);
            received_code = send(socket, send_buffer, sizeof(kermit_protocol_t), 0);
            
            free(send_buffer);         
        }

        //done
        if (received_buffer.type == showSpecificLineType) {
            kermit_protocol_t received_buffer_line;
            int receiveLine;
            receiveLine = getMessageFromAnotherProcess(socket, &received_buffer_line);
            receiveLine = getMessageFromAnotherProcess(socket, &received_buffer_line);

            if(received_buffer_line.type == linesType && received_buffer.source_address == clientAddress) {
                char fileContent[256] = "";
                showSpecificLineContentServer(received_buffer.data, atoi(received_buffer_line.data), fileContent);

                kermit_protocol_t *send_buffer;
                send_buffer = defineProtocol(clientAddress, serverAddress, fileContentType, fileContent, 0);
                received_code = send(socket, send_buffer, sizeof(kermit_protocol_t), 0);
                
                free(send_buffer);         
            }
        }

        //done
        if (received_buffer.type == showLinesBetweenType) {
            kermit_protocol_t received_buffer_line;
            int receiveLine;
            receiveLine = getMessageFromAnotherProcess(socket, &received_buffer_line);
            receiveLine = getMessageFromAnotherProcess(socket, &received_buffer_line);

            if(received_buffer_line.type == linesType && received_buffer.source_address == clientAddress) {
                char initialLine[100];
                char finalLine[100];
                char *lines = strtok(received_buffer_line.data, " ");
                strcpy(initialLine, lines);

                lines = strtok(NULL, " ");
                strcpy(finalLine, lines);
                char fileContent[256] = "";
                
                showLinesContentInRangeServer(received_buffer.data, atoi(initialLine), atoi(finalLine), fileContent);

                kermit_protocol_t *send_buffer;
                send_buffer = defineProtocol(clientAddress, serverAddress, fileContentType, fileContent, 0);
                received_code = send(socket, send_buffer, sizeof(kermit_protocol_t), 0);
                
                free(send_buffer);         
            }
        }

        //done
        if (received_buffer.type == editLineType && received_buffer.source_address == clientAddress) {
            kermit_protocol_t received_buffer_line;
            int receiveLine;
            receiveLine = getMessageFromAnotherProcess(socket, &received_buffer_line);
            receiveLine = getMessageFromAnotherProcess(socket, &received_buffer_line);

            if(received_buffer_line.type == linesType) {
                kermit_protocol_t received_buffer_content;
                int receiveContent;
                receiveContent = getMessageFromAnotherProcess(socket, &received_buffer_content);
                receiveContent = getMessageFromAnotherProcess(socket, &received_buffer_content);

                if (received_buffer_content.type == fileContentType) {
                    editSpecificLineContent(received_buffer.data, atoi(received_buffer_line.data), received_buffer_content.data); 

                    kermit_protocol_t *send_buffer;
                    send_buffer = defineProtocol(clientAddress, serverAddress, ACKType, "", 0);
                    received_code = send(socket, send_buffer, sizeof(kermit_protocol_t), 0);
                    
                    free(send_buffer);
                }
            }
        }

        //codifica resultado
        //envia resultado
        // 
    }

    return(0);
}