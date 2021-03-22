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
#include "functions.h"

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


char device[] = "lo";
int receive, receiveDiscard, received_code;
kermit_protocol_t received_buffer, received_discard_buffer;
char *emptyMessage = "";

int main() {
    int socket = ConexaoRawSocket(device);

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
            int sequence = 0;
            char *message = "";
            int received_code;

            received_code = sendMessage(socket, serverAddress, clientAddress, listCurrentDirectoryType, message, sequence);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, listCurrentDirectoryType, message, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                } else if(received_buffer.type == listDirectoryContentType && received_buffer.source_address == serverAddress) {
                    printf("%s", received_buffer.data);
                    received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                }

            } while(received_buffer.type != endTransmissionType || received_buffer.source_address != serverAddress);
            
            printf("Terminou transmissão\n");
            received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
            
        }
        
        //done
        if(!strcmp(command, changeDirectoryServer)) {
            char path[100];
            scanf("%s", path);

            int sequence = 0;
            char *message = "";
            int received_code;

            received_code = sendMessage(socket, serverAddress, clientAddress, changeDirectoryType, path, sequence);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, changeDirectoryType, path, sequence);
                }

            } while((received_buffer.type != ACKType || received_buffer.source_address != serverAddress) && received_buffer.type != ErrorType);

            if (received_buffer.type == ErrorType) {
                printf("ErrorCode: ");
                printf("%s\n", received_buffer.data);
            }
            
        }

        //done
        if(!strcmp(command, showFileContent)) {
            char file[100];
            scanf("%s", file);

            int sequence = 0;
            int received_code;
            char *message = file;
            char *newMessage = calloc(15, sizeof(char));
            int messageSize = strlen(message);
            int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            do {
                memset(newMessage, 0, 15);
                memcpy(newMessage, message, cuttedMessageSize);

                received_code = sendMessage(socket, serverAddress, clientAddress, showFileContentType, newMessage, sequence);

                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, showFileContentType, newMessage, sequence);
                    } else if(received_buffer.type == ErrorType) {
                        printf("Error: ");
                        printf("%s\n", received_buffer.data);
                    }

                } while(received_buffer.type != ACKType || received_buffer.source_address != serverAddress);
                
                sequence++;
                message += 15;
                messageSize -= 15;
                cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            } while (messageSize >= 1);

            received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence++);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                }

            } while(received_buffer.type != fileContentType || received_buffer.source_address != serverAddress);

            sequence = 0;
            printf("%s", received_buffer.data);
            received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);

            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                } else if(received_buffer.type == fileContentType && received_buffer.source_address == serverAddress) {
                    printf("%s", received_buffer.data);
                    received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                }

            } while(received_buffer.type != endTransmissionType || received_buffer.source_address != serverAddress);
            
            printf("Terminou transmissão\n");
            received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
        }

        //done
        if(!strcmp(command, showSpecificLine)) {
            char line[100];
            char file[100];
            scanf("%s", line);
            scanf("%s", file);

            int sequence = 0;
            int received_code;
            char *message = file;
            char *newMessage = calloc(15, sizeof(char));
            int messageSize = strlen(message);
            int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            do {
                memset(newMessage, 0, 15);
                memcpy(newMessage, message, cuttedMessageSize);

                received_code = sendMessage(socket, serverAddress, clientAddress, showSpecificLineType, newMessage, sequence);

                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, showSpecificLineType, newMessage, sequence);
                    } else if(received_buffer.type == ErrorType) {
                        printf("Error: ");
                        printf("%s\n", received_buffer.data);
                    }

                } while(received_buffer.type != ACKType || received_buffer.source_address != serverAddress);
                
                sequence++;
                message += 15;
                messageSize -= 15;
                cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            } while (messageSize >= 1);

            received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence++);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                }

            } while(received_buffer.type != ACKType || received_buffer.source_address != serverAddress);

            sequence = 0;
            received_code = sendMessage(socket, serverAddress, clientAddress, linesType, line, sequence);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, linesType, line, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                }

            } while(received_buffer.type != fileContentType || received_buffer.source_address != serverAddress);

            printf("%s", received_buffer.data);
            received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);

            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                } else if(received_buffer.type == fileContentType && received_buffer.source_address == serverAddress) {
                    printf("%s", received_buffer.data);
                    received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                }

            } while(received_buffer.type != endTransmissionType || received_buffer.source_address != serverAddress);
            
            printf("Terminou transmissão\n");
            received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
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

            int sequence = 0;
            int received_code;
            char *message = file;
            char *newMessage = calloc(15, sizeof(char));
            int messageSize = strlen(message);
            int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            do {
                memset(newMessage, 0, 15);
                memcpy(newMessage, message, cuttedMessageSize);

                received_code = sendMessage(socket, serverAddress, clientAddress, showLinesBetweenType, newMessage, sequence);

                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, showLinesBetweenType, newMessage, sequence);
                    } else if(received_buffer.type == ErrorType) {
                        printf("Error: ");
                        printf("%s\n", received_buffer.data);
                    }

                } while(received_buffer.type != ACKType || received_buffer.source_address != serverAddress);
                
                sequence++;
                message += 15;
                messageSize -= 15;
                cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            } while (messageSize >= 1);

            received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence++);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                }

            } while(received_buffer.type != ACKType || received_buffer.source_address != serverAddress);

            sequence = 0;
            received_code = sendMessage(socket, serverAddress, clientAddress, linesType, lines, sequence);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, linesType, lines, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                }

            } while(received_buffer.type != fileContentType || received_buffer.source_address != serverAddress);

            printf("%s", received_buffer.data);
            received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);

            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                } else if(received_buffer.type == fileContentType && received_buffer.source_address == serverAddress) {
                    printf("%s", received_buffer.data);
                    received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                }

            } while(received_buffer.type != endTransmissionType || received_buffer.source_address != serverAddress);
            
            printf("Terminou transmissão\n");
            received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);

            // kermit_protocol_t *sendFile;
            // kermit_protocol_t *sendLine;
            // sendFile = defineProtocol(serverAddress, clientAddress, showLinesBetweenType, file, 0);
            // sendLine = defineProtocol(serverAddress, clientAddress, linesType, lines, 1);
            // received_code = send(socket, sendFile, sizeof(kermit_protocol_t), 0);
            // received_code = send(socket, sendLine, sizeof(kermit_protocol_t), 0);

            // if (received_code == sizeof(kermit_protocol_t)) {
            //     while(1) {
            //         receive = getMessageFromAnotherProcess(socket, &received_buffer);
            //         receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    
            //         if (received_buffer.type == fileContentType && received_buffer.source_address == serverAddress) {
            //             printf("%s\n", received_buffer.data);
            //             break;
            //         }
            //     }
            // }
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