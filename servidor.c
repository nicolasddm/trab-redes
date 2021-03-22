#include <dirent.h> 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "comandos.h"
#include "functions.h"
#include "socket.h"
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


int receive, received_code;
char device[] = "lo";
kermit_protocol_t received_buffer;
char *emptyMessage = "";

int main() {
    int socket = ConexaoRawSocket(device);
    char directoryPath[100];
    time_t startTime, endTime;

    while (1) {
        
        receive = getMessageFromAnotherProcess(socket, &received_buffer);
        receive = getMessageFromAnotherProcess(socket, &received_buffer);

        if(!checkParity(&received_buffer)) {
            char *message = "";
            int sequence = 0b0000;
            received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
        }

        if (received_buffer.type == changeDirectoryType && received_buffer.source_address == clientAddress) {
            int chdirReturn = changeDirectory(received_buffer.data);
            
            char *message = "";
            int sequence = 0b0000;
            
            if (chdirReturn != 0) {
                char errorCodeStr[4] = "";
                sprintf(errorCodeStr, "%d\n", chdirReturn);
                received_code = sendMessage(socket, clientAddress, serverAddress, ErrorType, errorCodeStr, sequence);
            } else {
                received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            }
        }

        if (received_buffer.type == listCurrentDirectoryType && received_buffer.source_address == clientAddress) {
            int sequence = 0b0000;
            char *emptyMessage = "";
            int received_code;
            
            char newMessage[15];
            char directoryFilesList[256] = "";
            memset(directoryFilesList, 0, 256);
            int errorCode = listCurrentDirectoryFiles(directoryFilesList);
            if (errorCode != 0) {
                char errorCodeStr[4] = "";
                sprintf(errorCodeStr, "%d\n", errorCode);
                received_code = sendMessage(socket, clientAddress, serverAddress, ErrorType, errorCodeStr, sequence);
            } else {
               
                char *message = directoryFilesList;
                int messageSize = strlen(message);
                int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

                do {
                    memset(newMessage, 0, 15);
                    memcpy(newMessage, message, cuttedMessageSize);

                    received_code = sendMessage(socket, clientAddress, serverAddress, listDirectoryContentType, newMessage, sequence);
                    time(&startTime);
                    do {
                        receive = getMessageFromAnotherProcess(socket, &received_buffer);
                        receive = getMessageFromAnotherProcess(socket, &received_buffer);

                        if(!checkParity(&received_buffer)) {
                            received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                        } else if(received_buffer.type == NACKType && received_buffer.source_address == clientAddress) {
                            received_code = sendMessage(socket, clientAddress, serverAddress, listDirectoryContentType, newMessage, sequence);
                        } else if(received_buffer.type == ErrorType) {
                            printf("Error: ");
                            printf("%s\n", received_buffer.data);
                        }
                        verifyTimeout(startTime, endTime);

                    } while(received_buffer.type != ACKType || received_buffer.source_address != clientAddress);
                    
                    sequence++;
                    message += 15;
                    messageSize -= 15;
                    cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

                } while (messageSize >= 1);
                
                received_code = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence++);
                time(&startTime);

                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(!checkParity(&received_buffer)) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if(received_buffer.type == NACKType) {
                        printf("NACK, reenviando\n");
                        received_code = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence);
                    } else if(received_buffer.type == ErrorType) {
                        printf("Error: ");
                        printf("%s\n", received_buffer.data);
                    }
                    verifyTimeout(startTime, endTime);

                } while(received_buffer.type != ACKType || received_buffer.source_address != clientAddress);
            }
            
        }

        if (received_buffer.type == showFileContentType && received_buffer.source_address == clientAddress) {
            int sequence = 0b0000;
            char *message = "";
            int received_code;
            char fileName[256] = "";
            strcat(fileName, received_buffer.data);

            //ficar escutando o arquivo todo
            received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    received_code = sendMessage(socket, clientAddress, serverAddress, NACKType, message, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                } else if(received_buffer.type == showFileContentType && received_buffer.source_address == clientAddress) {
                    strcat(fileName, received_buffer.data);
                    received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while(received_buffer.type != endTransmissionType || received_buffer.source_address != clientAddress);

            //pegar o conteúdo
            char fileContent[256] = "";
            int errorCode = showFileContentServer(fileName, fileContent);
            if (errorCode != 0) {
                char errorCodeStr[4] = "";
                sprintf(errorCodeStr, "%d\n", errorCode);
                received_code = sendMessage(socket, clientAddress, serverAddress, ErrorType, errorCodeStr, sequence);
            } else {
                char *newMessage = calloc(15, sizeof(char));
                message = fileContent;
                int messageSize = strlen(message);
                int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;
                
                //ficar enviando o conteúdo
                do {
                    memset(newMessage, 0, 15);
                    memcpy(newMessage, message, cuttedMessageSize);
                    received_code = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);
                    time(&startTime);
                    do {
                        receive = getMessageFromAnotherProcess(socket, &received_buffer);
                        receive = getMessageFromAnotherProcess(socket, &received_buffer);

                        if(!checkParity(&received_buffer)) {
                            received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                        } else if(received_buffer.type == NACKType && received_buffer.source_address == clientAddress) {
                            received_code = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);
                        } else if(received_buffer.type == ErrorType) {
                            printf("Error: ");
                            printf("%s\n", received_buffer.data);
                        }
                        verifyTimeout(startTime, endTime);

                    } while(received_buffer.type != ACKType || received_buffer.source_address != clientAddress);
                    
                    sequence++;
                    message += 15;
                    messageSize -= 15;
                    cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

                } while (messageSize >= 1);
                
                received_code = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence++);
                time(&startTime);
                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(!checkParity(&received_buffer)) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if(received_buffer.type == NACKType) {
                        printf("NACK, reenviando\n");
                        received_code = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence);
                    } else if(received_buffer.type == ErrorType) {
                        printf("Error: ");
                        printf("%s\n", received_buffer.data);
                    }
                    verifyTimeout(startTime, endTime);

                } while(received_buffer.type != ACKType || received_buffer.source_address != clientAddress);
            }      
        }

        if (received_buffer.type == showSpecificLineType && received_buffer.source_address == clientAddress) {
            int sequence = 0b0000;
            char *message = "";
            int received_code;
            int line;
            char fileName[256] = "";
            strcat(fileName, received_buffer.data);

            //ficar escutando o arquivo todo
            received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    received_code = sendMessage(socket, clientAddress, serverAddress, NACKType, message, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                } else if(received_buffer.type == showSpecificLineType && received_buffer.source_address == clientAddress) {
                    strcat(fileName, received_buffer.data);
                    received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while(received_buffer.type != endTransmissionType || received_buffer.source_address != clientAddress);

            received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (received_buffer.type == linesType && received_buffer.source_address == clientAddress) {
                    line = atoi(received_buffer.data);
                }
                verifyTimeout(startTime, endTime);

            } while(received_buffer.type != linesType || received_buffer.source_address != clientAddress);

            
            //pegar o conteúdo
            char fileContent[256] = "";
            int errorCode = showSpecificLineContentServer(fileName, line, fileContent);
            if (errorCode != 0) {
                char errorCodeStr[4] = "";
                sprintf(errorCodeStr, "%d\n", errorCode);
                received_code = sendMessage(socket, clientAddress, serverAddress, ErrorType, errorCodeStr, sequence);
            } else {
                
                char *newMessage = calloc(15, sizeof(char));
                message = fileContent;
                int messageSize = strlen(message);
                int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;
            
                do {
                    memset(newMessage, 0, 15);
                    memcpy(newMessage, message, cuttedMessageSize);
                    received_code = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);
                    time(&startTime);
                    do {
                        receive = getMessageFromAnotherProcess(socket, &received_buffer);
                        receive = getMessageFromAnotherProcess(socket, &received_buffer);

                        if(!checkParity(&received_buffer)) {
                            received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                        } else if(received_buffer.type == NACKType && received_buffer.source_address == clientAddress) {
                            received_code = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);
                        } else if(received_buffer.type == ErrorType) {
                            printf("Error: ");
                            printf("%s\n", received_buffer.data);
                        }
                        verifyTimeout(startTime, endTime);

                    } while(received_buffer.type != ACKType || received_buffer.source_address != clientAddress);
                    
                    sequence++;
                    message += 15;
                    messageSize -= 15;
                    cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

                } while (messageSize >= 1);
                
                received_code = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence++);
                time(&startTime);
                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    
                    if(!checkParity(&received_buffer)) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if(received_buffer.type == NACKType) {
                        printf("NACK, reenviando\n");
                        received_code = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence);
                    } else if(received_buffer.type == ErrorType) {
                        printf("Error: ");
                        printf("%s\n", received_buffer.data);
                    }
                    verifyTimeout(startTime, endTime);

                } while(received_buffer.type != ACKType || received_buffer.source_address != clientAddress);
            }
        }

        if (received_buffer.type == showLinesBetweenType && received_buffer.source_address == clientAddress) {
            int sequence = 0b0000;
            char *message = "";
            int received_code;
            int line;
            char fileName[256] = "";
            strcat(fileName, received_buffer.data);

            //ficar escutando o arquivo todo
            received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (received_buffer.type == NACKType && received_buffer.source_address == clientAddress) {
                    printf("isNack, Tentando de novo\n");
                    received_code = sendMessage(socket, clientAddress, serverAddress, NACKType, message, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                } else if(received_buffer.type == showLinesBetweenType && received_buffer.source_address == clientAddress) {
                    strcat(fileName, received_buffer.data);
                    received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while(received_buffer.type != endTransmissionType || received_buffer.source_address != clientAddress);

            received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (received_buffer.type == linesType && received_buffer.source_address == clientAddress) {
                    line = atoi(received_buffer.data);
                }
                verifyTimeout(startTime, endTime);

            } while(received_buffer.type != linesType || received_buffer.source_address != clientAddress);
            
            //pegar o conteúdo
            char initialLine[100];
            char finalLine[100];
            char *lines = strtok(received_buffer.data, " ");

            strcpy(initialLine, lines);
            lines = strtok(NULL, " ");
            strcpy(finalLine, lines);
            char fileContent[256] = "";

            int errorCode = showLinesContentInRangeServer(fileName, atoi(initialLine), atoi(finalLine), fileContent);
            if (errorCode != 0) {
                printf("%d\n", errorCode);
                char errorCodeStr[4] = "";
                sprintf(errorCodeStr, "%d\n", errorCode);
                printf("%s\n", errorCodeStr);
                received_code = sendMessage(socket, clientAddress, serverAddress, ErrorType, errorCodeStr, sequence);
            } else {
                char *newMessage = calloc(15, sizeof(char));
                message = fileContent;
                int messageSize = strlen(message);
                int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;
                
                //ficar enviando o conteúdo
                do {
                    memset(newMessage, 0, 15);
                    memcpy(newMessage, message, cuttedMessageSize);
                    received_code = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);
                    time(&startTime);

                    do {
                        receive = getMessageFromAnotherProcess(socket, &received_buffer);
                        receive = getMessageFromAnotherProcess(socket, &received_buffer);

                        if(!checkParity(&received_buffer)) {
                            received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                        } else if(received_buffer.type == NACKType && received_buffer.source_address == clientAddress) {
                            received_code = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);
                        } else if(received_buffer.type == ErrorType) {
                            printf("Error: ");
                            printf("%s\n", received_buffer.data);
                        }
                        verifyTimeout(startTime, endTime);

                    } while(received_buffer.type != ACKType || received_buffer.source_address != clientAddress);
                    
                    sequence++;
                    message += 15;
                    messageSize -= 15;
                    cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

                } while (messageSize >= 1);
                
                received_code = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence++);
                time(&startTime);
                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    
                    if(!checkParity(&received_buffer)) {
                        received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if(received_buffer.type == NACKType) {
                        printf("NACK, reenviando\n");
                        received_code = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence);
                    } else if(received_buffer.type == ErrorType) {
                        printf("Error: ");
                        printf("%s\n", received_buffer.data);
                    }
                    verifyTimeout(startTime, endTime);

                } while(received_buffer.type != ACKType || received_buffer.source_address != clientAddress);
            }


        }

        if (received_buffer.type == editLineType && received_buffer.source_address == clientAddress) {
            int sequence = 0b0000;
            char *message = "";
            int received_code;
            int line;
            char fileName[256] = "";
            strcat(fileName, received_buffer.data);

            //ficar escutando o arquivo todo
            received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    received_code = sendMessage(socket, clientAddress, serverAddress, NACKType, message, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                } else if(received_buffer.type == editLineType && received_buffer.source_address == clientAddress) {
                    strcat(fileName, received_buffer.data);
                    received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while(received_buffer.type != endTransmissionType || received_buffer.source_address != clientAddress);
            
            received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (received_buffer.type == linesType && received_buffer.source_address == clientAddress) {
                    line = atoi(received_buffer.data);
                }
                verifyTimeout(startTime, endTime);

            } while(received_buffer.type != linesType || received_buffer.source_address != clientAddress);

            char newContent[256] = "";
            received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(!checkParity(&received_buffer)) {
                    received_code = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    received_code = sendMessage(socket, clientAddress, serverAddress, NACKType, message, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                } else if(received_buffer.type == fileContentType && received_buffer.source_address == clientAddress) {
                    strcat(newContent, received_buffer.data);
                    received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while(received_buffer.type != endTransmissionType || received_buffer.source_address != clientAddress);

            int errorCode = editSpecificLineContent(fileName, line, newContent);
            if (errorCode != 0) {
                printf("%d\n", errorCode);
                char errorCodeStr[4] = "";
                sprintf(errorCodeStr, "%d\n", errorCode);
                printf("%s\n", errorCodeStr);
                received_code = sendMessage(socket, clientAddress, serverAddress, ErrorType, errorCodeStr, sequence);
            } else {
                received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            }

        }

    }

    return(0);
}