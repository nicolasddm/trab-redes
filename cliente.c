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
#include <time.h>

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
            char directory[100] = "";
            scanf("%s", directory);
            int errorCode = changeDirectory(directory);
            if(errorCode != 0){
                printf("ErrorCode: ");
                printf("%d\n", errorCode);
            }
        }

        //done
        if(!strcmp(command, listCurrentDirectoryClient)) {
            char directoryFilesList[256] = "";
            int errorCode = listCurrentDirectoryFiles(directoryFilesList);
            if(errorCode != 0){
                printf("ErrorCode: ");
                printf("%d\n", errorCode);
            } else {
                printf("%s", directoryFilesList);
            }
        }
        
        //done
        if(!strcmp(command, listCurrentDirectoryServer)) {
            int sequence = 0b0000;
            char *message = "";
            int received_code;

            received_code = sendMessage(socket, serverAddress, clientAddress, listCurrentDirectoryType, message, sequence);

            time_t startTime, endTime;
            time(&startTime);

            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, listCurrentDirectoryType, message, sequence);
                } else if(received_buffer.type == listDirectoryContentType && received_buffer.source_address == serverAddress) {
                    printf("%s", received_buffer.data);
                    received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while((received_buffer.type != endTransmissionType || received_buffer.source_address != serverAddress) && received_buffer.type != ErrorType);
            if(received_buffer.type == ErrorType) {
                printf("ErrorCode: ");
                printf("%s\n", received_buffer.data);
            } else {
                printf("Terminou transmissão\n");
                received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
            }
            
        }
        
        //done
        if(!strcmp(command, changeDirectoryServer)) {
            char path[100];
            scanf("%s", path);

            int sequence = 0b0000;
            char *message = "";
            int received_code;

            received_code = sendMessage(socket, serverAddress, clientAddress, changeDirectoryType, path, sequence);
            time_t startTime, endTime;
            time(&startTime);

            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                
                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, changeDirectoryType, path, sequence);
                }
                verifyTimeout(startTime, endTime);

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

            int sequence = 0b0000;
            int received_code;
            char *message = file;
            char *newMessage = calloc(15, sizeof(char));
            int messageSize = strlen(message);
            int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;
            time_t startTime, endTime;

            do {
                memset(newMessage, 0, 15);
                memcpy(newMessage, message, cuttedMessageSize);

                received_code = sendMessage(socket, serverAddress, clientAddress, showFileContentType, newMessage, sequence);
                
                time(&startTime);

                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(!checkParity(&received_buffer)) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if(received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, showFileContentType, newMessage, sequence);
                    }
                    verifyTimeout(startTime, endTime);

                } while(received_buffer.type != ACKType || received_buffer.source_address != serverAddress);
                
                sequence++;
                message += 15;
                messageSize -= 15;
                cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            } while (messageSize >= 1);

            received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence++);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                
                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while((received_buffer.type != fileContentType || received_buffer.source_address != serverAddress) && received_buffer.type != ErrorType);

            if(received_buffer.type == ErrorType) {
                printf("ErrorCode: ");
                printf("%s\n", received_buffer.data);
            } else {
                sequence = 0b0000;
                received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                time(&startTime);
                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(!checkParity(&received_buffer)) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                        printf("isNack, Tentando de novo\n");
                        received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                    } else if(received_buffer.type == fileContentType && received_buffer.source_address == serverAddress) {
                        printf("%s", received_buffer.data);
                        received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                    }
                    verifyTimeout(startTime, endTime);

                } while(received_buffer.type != endTransmissionType || received_buffer.source_address != serverAddress);
                
                printf("Terminou transmissão\n");
                received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
            }

        }

        //done
        if(!strcmp(command, showSpecificLine)) {
            char line[100];
            char file[100];
            scanf("%s", line);
            scanf("%s", file);

            int sequence = 0b0000;
            int received_code;
            char *message = file;
            char *newMessage = calloc(15, sizeof(char));
            int messageSize = strlen(message);
            int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;
            time_t startTime, endTime;

            do {
                memset(newMessage, 0, 15);
                memcpy(newMessage, message, cuttedMessageSize);

                received_code = sendMessage(socket, serverAddress, clientAddress, showSpecificLineType, newMessage, sequence);
                time(&startTime);
                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(!checkParity(&received_buffer)) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if(received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, showSpecificLineType, newMessage, sequence);
                    }
                    verifyTimeout(startTime, endTime);

                } while(received_buffer.type != ACKType || received_buffer.source_address != serverAddress);
                
                sequence++;
                message += 15;
                messageSize -= 15;
                cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            } while (messageSize >= 1);

            received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence++);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while(received_buffer.type != ACKType || received_buffer.source_address != serverAddress);

            
            sequence = 0b0000;
            received_code = sendMessage(socket, serverAddress, clientAddress, linesType, line, sequence);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, linesType, line, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while((received_buffer.type != fileContentType || received_buffer.source_address != serverAddress) && received_buffer.type != ErrorType);

            if(received_buffer.type == ErrorType) {
                printf("ErrorCode: ");
                printf("%s\n", received_buffer.data);
            } else {
                printf("%s", received_buffer.data);
                received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                time(&startTime);
                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(!checkParity(&received_buffer)) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                        printf("isNack, Tentando de novo\n");
                        received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                    } else if(received_buffer.type == fileContentType && received_buffer.source_address == serverAddress) {
                        printf("%s", received_buffer.data);
                        received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                    }
                    verifyTimeout(startTime, endTime);

                } while(received_buffer.type != endTransmissionType || received_buffer.source_address != serverAddress);
                
                printf("Terminou transmissão\n");
                received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
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

            int sequence = 0b0000;
            int received_code;
            char *message = file;
            char *newMessage = calloc(15, sizeof(char));
            int messageSize = strlen(message);
            int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;
            time_t startTime, endTime;

            do {
                memset(newMessage, 0, 15);
                memcpy(newMessage, message, cuttedMessageSize);

                received_code = sendMessage(socket, serverAddress, clientAddress, showLinesBetweenType, newMessage, sequence);
                time(&startTime);

                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(!checkParity(&received_buffer)) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if(received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, showLinesBetweenType, newMessage, sequence);
                    }
                    verifyTimeout(startTime, endTime);

                } while(received_buffer.type != ACKType || received_buffer.source_address != serverAddress);
                
                sequence++;
                message += 15;
                messageSize -= 15;
                cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            } while (messageSize >= 1);

            received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence++);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while(received_buffer.type != ACKType || received_buffer.source_address != serverAddress);

            sequence = 0b0000;
            received_code = sendMessage(socket, serverAddress, clientAddress, linesType, lines, sequence);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, linesType, lines, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while((received_buffer.type != fileContentType || received_buffer.source_address != serverAddress) && received_buffer.type != ErrorType);

            if(received_buffer.type == ErrorType) {
                printf("ErrorCode: ");
                printf("%s\n", received_buffer.data);
            } else {

                printf("%s", received_buffer.data);
                received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                time(&startTime);
                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(!checkParity(&received_buffer)) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                        printf("isNack, Tentando de novo\n");
                        received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                    } else if(received_buffer.type == fileContentType && received_buffer.source_address == serverAddress) {
                        printf("%s", received_buffer.data);
                        received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
                    }
                    verifyTimeout(startTime, endTime);

                } while(received_buffer.type != endTransmissionType || received_buffer.source_address != serverAddress);
                
                printf("Terminou transmissão\n");
                received_code = sendMessage(socket, serverAddress, clientAddress, ACKType, message, sequence);
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

            int sequence = 0b0000;
            int received_code;
            char *message = file;
            char *newMessage = calloc(15, sizeof(char));
            int messageSize = strlen(message);
            int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;
            time_t startTime, endTime;

            do {
                memset(newMessage, 0, 15);
                memcpy(newMessage, message, cuttedMessageSize);

                received_code = sendMessage(socket, serverAddress, clientAddress, editLineType, newMessage, sequence);
                time(&startTime);
                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(!checkParity(&received_buffer)) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if(received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, editLineType, newMessage, sequence);
                    }
                    verifyTimeout(startTime, endTime);

                } while(received_buffer.type != ACKType || received_buffer.source_address != serverAddress);
                
                sequence++;
                message += 15;
                messageSize -= 15;
                cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            } while (messageSize >= 1);

            received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence++);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while(received_buffer.type != ACKType || received_buffer.source_address != serverAddress);
            
            sequence = 0b0000;
            received_code = sendMessage(socket, serverAddress, clientAddress, linesType, line, sequence);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, linesType, line, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while(received_buffer.type != ACKType || received_buffer.source_address != serverAddress);

            message = newContent;
            messageSize = strlen(message);
            cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            do {
                memset(newMessage, 0, 15);
                memcpy(newMessage, message, cuttedMessageSize);

                received_code = sendMessage(socket, serverAddress, clientAddress, fileContentType, newMessage, sequence);
                time(&startTime);
                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(!checkParity(&received_buffer)) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if(received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, fileContentType, newMessage, sequence);
                    }
                    verifyTimeout(startTime, endTime);

                } while(received_buffer.type != ACKType || received_buffer.source_address != serverAddress);
                
                sequence++;
                message += 15;
                messageSize -= 15;
                cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            } while (messageSize >= 1);

            received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence++);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, serverAddress, clientAddress, endTransmissionType, emptyMessage, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while((received_buffer.type != ACKType || received_buffer.source_address != serverAddress) && received_buffer.type != ErrorType);

            if(received_buffer.type == ErrorType) {
                printf("ErrorCode: ");
                printf("%s\n", received_buffer.data);
            }
        }
    }
    
    return(0);
}