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
char changeDirectoryClient[100] = "lcd";
char listCurrentDirectoryServer[100] = "ls";
char listCurrentDirectoryClient[100] = "lls";
char showFileContent[100] = "ver";
char showSpecificLine[100] = "linha";
char showLinesBetween[100] = "linhas";
char editLine[100] = "edit";

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

char *getNewContent(char *content) {    
    fgets(content, 100, stdin);

    char *newContent = cleanNewContent(content);
    return newContent;
}

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

char device[] = "lo";
int receive, receiveDiscard, received_code;
kermit_protocol_t received_buffer, received_discard_buffer;


int main() {
    int socket = ConexaoRawSocket(device);
    printf("%d\n", socket);

    kermit_protocol_t *cliente = (kermit_protocol_t *)calloc(1, sizeof(kermit_protocol_t));
    strcpy(cliente->data, "salve");

    while(1) {
        char directoryPath[100];
        char command[100];
        printf("%s ", getcwd(directoryPath, 100));
        scanf("%s", command);
       
        //done
        if(!strcmp(command, changeDirectoryClient)) {
            char directory[100];
            scanf("%s", directory);
            changeDirectory(directory);
        }

        //done
        if(!strcmp(command, listCurrentDirectoryClient)) {
            char directoryFilesList[256];
            listCurrentDirectoryFiles(directoryFilesList);
            printf("%s", directoryFilesList);
        }
        
        //done
        if(!strcmp(command, listCurrentDirectoryServer)) {
            kermit_protocol_t *send_buffer;
            send_buffer = defineProtocol(serverAddress, clientAddress, listCurrentDirectoryType, "", 0);
            received_code = send(socket, send_buffer, sizeof(kermit_protocol_t), 0);
            free(send_buffer);

            if (received_code == sizeof(kermit_protocol_t)) {
                while(1) {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receiveDiscard = getMessageFromAnotherProcess(socket, &received_discard_buffer);
                    if (received_buffer.type == listDirectoryContentType && received_buffer.source_address == serverAddress) {
                        printf("%s\n", received_buffer.data);
                        break;
                    }
                }
            }
        }
        
        //todo
        if(!strcmp(command, changeDirectoryServer)) {
            char path[100];
            scanf("%s", path);

            kermit_protocol_t *sendPath;
            sendPath = defineProtocol(serverAddress, clientAddress, changeDirectoryType, path, 0);            
            received_code = send(socket, sendPath, sizeof(kermit_protocol_t), 0);

            if (received_code == sizeof(kermit_protocol_t)) {
                while(1) {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    
                    if (received_buffer.type == ACKType && received_buffer.source_address == serverAddress) {
                        printf("%s\n", received_buffer.data);
                        break;
                    }
                }
            }
            
        }

        //done
        if(!strcmp(command, showFileContent)) {
            char file[100];
            scanf("%s", file);

            kermit_protocol_t *sendFile;
            sendFile = defineProtocol(serverAddress, clientAddress, showFileContentType, file, 0);            
            received_code = send(socket, sendFile, sizeof(kermit_protocol_t), 0);

            if (received_code == sizeof(kermit_protocol_t)) {
                while(1) {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    
                    if (received_buffer.type == fileContentType && received_buffer.source_address == serverAddress) {
                        printf("%s\n", received_buffer.data);
                        break;
                    }
                }
            }
        }

        //done
        if(!strcmp(command, showSpecificLine)) {
            char line[100];
            char file[100];
            scanf("%s", line);
            scanf("%s", file);

            kermit_protocol_t *sendFile;
            kermit_protocol_t *sendLine;
            sendFile = defineProtocol(serverAddress, clientAddress, showSpecificLineType, file, 0);
            sendLine = defineProtocol(serverAddress, clientAddress, linesType, line, 1);
            received_code = send(socket, sendFile, sizeof(kermit_protocol_t), 0);
            received_code = send(socket, sendLine, sizeof(kermit_protocol_t), 0);

            if (received_code == sizeof(kermit_protocol_t)) {
                while(1) {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    
                    if (received_buffer.type == fileContentType && received_buffer.source_address == serverAddress) {
                        printf("%s\n", received_buffer.data);
                        break;
                    }
                }
            }
        }

        //done
        if(!strcmp(command, showLinesBetween)) {
            char initialLine[100];
            char finalLine[100];
            char file[100];
            scanf("%s", initialLine);
            scanf("%s", finalLine);
            scanf("%s", file);

            char lines[100] = "";
            strcat(lines, initialLine);
            strcat(lines, " ");
            strcat(lines, finalLine);

            kermit_protocol_t *sendFile;
            kermit_protocol_t *sendLine;
            sendFile = defineProtocol(serverAddress, clientAddress, showLinesBetweenType, file, 0);
            sendLine = defineProtocol(serverAddress, clientAddress, linesType, lines, 1);
            received_code = send(socket, sendFile, sizeof(kermit_protocol_t), 0);
            received_code = send(socket, sendLine, sizeof(kermit_protocol_t), 0);

            if (received_code == sizeof(kermit_protocol_t)) {
                while(1) {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    
                    if (received_buffer.type == fileContentType && received_buffer.source_address == serverAddress) {
                        printf("%s\n", received_buffer.data);
                        break;
                    }
                }
            }
        }

        //done
        if(!strcmp(command, editLine)) {
            char line[100];
            char file[100];
            
            scanf("%s", line);
            scanf("%s", file);

            char newContent[100] = "";
            getNewContent(newContent);
            
            kermit_protocol_t *sendFile;
            kermit_protocol_t *sendLine;
            kermit_protocol_t *sendNewContent;
            sendFile = defineProtocol(serverAddress, clientAddress, editLineType, file, 0);
            sendLine = defineProtocol(serverAddress, clientAddress, linesType, line, 1);
            sendNewContent = defineProtocol(serverAddress, clientAddress, fileContentType, newContent, 2);
            
            received_code = send(socket, sendFile, sizeof(kermit_protocol_t), 0);
            received_code = send(socket, sendLine, sizeof(kermit_protocol_t), 0);
            received_code = send(socket, sendNewContent, sizeof(kermit_protocol_t), 0);


            if (received_code == sizeof(kermit_protocol_t)) {
                while(1) {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    
                    if (received_buffer.type == ACKType && received_buffer.source_address == serverAddress) {
                        printf("Editado com sucesso!\n");
                        break;
                    }
                }
            }
        }

        
    }
    
    return(0);
}