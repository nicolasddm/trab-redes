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


int receive, sendedCode;
char device[] = "lo";
kermit_type bufferListened;
char *emptyMessage = "";

int main() {
    int socket = ConexaoRawSocket(device);
    char directoryPath[100];
    time_t startTime, endTime;

    while (1) {
        
        receive = getMessage(socket, &bufferListened);
        receive = getMessage(socket, &bufferListened);

        if(!checkParity(&bufferListened)) {
            char *message = "";
            int sequence = 0b0000;
            sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
        }

        if (bufferListened.type == changeDirectoryType && bufferListened.sourceAddress == clientAddress) {
            int chdirReturn = changeDirectory(bufferListened.data);
            
            char *message = "";
            int sequence = 0b0000;
            
            if (chdirReturn != 0) {
                char errorCodeStr[4] = "";
                sprintf(errorCodeStr, "%d\n", chdirReturn);
                sendedCode = sendMessage(socket, clientAddress, serverAddress, ErrorType, errorCodeStr, sequence);
            } else {
                sendedCode = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            }
        }

        if (bufferListened.type == listCurrentDirectoryType && bufferListened.sourceAddress == clientAddress) {
            int sequence = 0b0000;
            char *emptyMessage = "";
            int sendedCode;
            
            char newMessage[15];
            char directoryFilesList[256] = "";
            memset(directoryFilesList, 0, 256);
            int errorCode = listCurrentDirectoryFiles(directoryFilesList);
            if (errorCode != 0) {
                char errorCodeStr[4] = "";
                sprintf(errorCodeStr, "%d\n", errorCode);
                sendedCode = sendMessage(socket, clientAddress, serverAddress, ErrorType, errorCodeStr, sequence);
            } else {
               
                char *message = directoryFilesList;
                int messageSize = strlen(message);
                int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

                do {
                    memset(newMessage, 0, 15);
                    memcpy(newMessage, message, cuttedMessageSize);

                    sendedCode = sendMessage(socket, clientAddress, serverAddress, listDirectoryContentType, newMessage, sequence);
                    time(&startTime);
                    do {
                        receive = getMessage(socket, &bufferListened);
                        receive = getMessage(socket, &bufferListened);

                        if(!checkParity(&bufferListened)) {
                            sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                        } else if(bufferListened.type == NACKType && bufferListened.sourceAddress == clientAddress) {
                            sendedCode = sendMessage(socket, clientAddress, serverAddress, listDirectoryContentType, newMessage, sequence);
                        }
                        verifyTimeout(startTime, endTime);

                    } while(bufferListened.type != ACKType || bufferListened.sourceAddress != clientAddress);
                    
                    sequence++;
                    message += 15;
                    messageSize -= 15;
                    cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

                } while (messageSize >= 1);
                
                sendedCode = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence++);
                time(&startTime);

                do {
                    receive = getMessage(socket, &bufferListened);
                    receive = getMessage(socket, &bufferListened);

                    if(!checkParity(&bufferListened)) {
                        sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if(bufferListened.type == NACKType) {
                        printf("NACK, reenviando\n");
                        sendedCode = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence);
                    }
                    verifyTimeout(startTime, endTime);

                } while(bufferListened.type != ACKType || bufferListened.sourceAddress != clientAddress);
            }
            
        }

        if (bufferListened.type == showFileContentType && bufferListened.sourceAddress == clientAddress) {
            int sequence = 0b0000;
            char *message = "";
            int sendedCode;
            char fileName[256] = "";
            strcat(fileName, bufferListened.data);

            //ficar escutando o arquivo todo
            sendedCode = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessage(socket, &bufferListened);
                receive = getMessage(socket, &bufferListened);

                if(!checkParity(&bufferListened)) {
                    sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (bufferListened.type == NACKType && bufferListened.sourceAddress == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    sendedCode = sendMessage(socket, clientAddress, serverAddress, NACKType, message, sequence);
                } else if(bufferListened.type == showFileContentType && bufferListened.sourceAddress == clientAddress) {
                    strcat(fileName, bufferListened.data);
                    sendedCode = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while(bufferListened.type != endTransmissionType || bufferListened.sourceAddress != clientAddress);

            //pegar o conteúdo
            char fileContent[256] = "";
            int errorCode = showFileContentServer(fileName, fileContent);
            if (errorCode != 0) {
                char errorCodeStr[4] = "";
                sprintf(errorCodeStr, "%d\n", errorCode);
                sendedCode = sendMessage(socket, clientAddress, serverAddress, ErrorType, errorCodeStr, sequence);
            } else {
                char *newMessage = calloc(15, sizeof(char));
                message = fileContent;
                int messageSize = strlen(message);
                int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;
                
                //ficar enviando o conteúdo
                do {
                    memset(newMessage, 0, 15);
                    memcpy(newMessage, message, cuttedMessageSize);
                    sendedCode = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);
                    time(&startTime);
                    do {
                        receive = getMessage(socket, &bufferListened);
                        receive = getMessage(socket, &bufferListened);

                        if(!checkParity(&bufferListened)) {
                            sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                        } else if(bufferListened.type == NACKType && bufferListened.sourceAddress == clientAddress) {
                            sendedCode = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);
                        }
                        verifyTimeout(startTime, endTime);

                    } while(bufferListened.type != ACKType || bufferListened.sourceAddress != clientAddress);
                    
                    sequence++;
                    message += 15;
                    messageSize -= 15;
                    cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

                } while (messageSize >= 1);
                
                sendedCode = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence++);
                time(&startTime);
                do {
                    receive = getMessage(socket, &bufferListened);
                    receive = getMessage(socket, &bufferListened);

                    if(!checkParity(&bufferListened)) {
                        sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if(bufferListened.type == NACKType) {
                        printf("NACK, reenviando\n");
                        sendedCode = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence);
                    }
                    verifyTimeout(startTime, endTime);

                } while(bufferListened.type != ACKType || bufferListened.sourceAddress != clientAddress);
            }      
        }

        if (bufferListened.type == showSpecificLineType && bufferListened.sourceAddress == clientAddress) {
            int sequence = 0b0000;
            char *message = "";
            int sendedCode;
            int line;
            char fileName[256] = "";
            strcat(fileName, bufferListened.data);

            //ficar escutando o arquivo todo
            sendedCode = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessage(socket, &bufferListened);
                receive = getMessage(socket, &bufferListened);

                if(!checkParity(&bufferListened)) {
                    sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (bufferListened.type == NACKType && bufferListened.sourceAddress == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    sendedCode = sendMessage(socket, clientAddress, serverAddress, NACKType, message, sequence);
                } else if(bufferListened.type == showSpecificLineType && bufferListened.sourceAddress == clientAddress) {
                    strcat(fileName, bufferListened.data);
                    sendedCode = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while(bufferListened.type != endTransmissionType || bufferListened.sourceAddress != clientAddress);

            sendedCode = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessage(socket, &bufferListened);
                receive = getMessage(socket, &bufferListened);

                if(!checkParity(&bufferListened)) {
                    sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (bufferListened.type == linesType && bufferListened.sourceAddress == clientAddress) {
                    line = atoi(bufferListened.data);
                }
                verifyTimeout(startTime, endTime);

            } while(bufferListened.type != linesType || bufferListened.sourceAddress != clientAddress);

            
            //pegar o conteúdo
            char fileContent[256] = "";
            int errorCode = showSpecificLineContentServer(fileName, line, fileContent);
            if (errorCode != 0) {
                char errorCodeStr[4] = "";
                sprintf(errorCodeStr, "%d\n", errorCode);
                sendedCode = sendMessage(socket, clientAddress, serverAddress, ErrorType, errorCodeStr, sequence);
            } else {
                
                char *newMessage = calloc(15, sizeof(char));
                message = fileContent;
                int messageSize = strlen(message);
                int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;
            
                do {
                    memset(newMessage, 0, 15);
                    memcpy(newMessage, message, cuttedMessageSize);
                    sendedCode = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);
                    time(&startTime);
                    do {
                        receive = getMessage(socket, &bufferListened);
                        receive = getMessage(socket, &bufferListened);

                        if(!checkParity(&bufferListened)) {
                            sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                        } else if(bufferListened.type == NACKType && bufferListened.sourceAddress == clientAddress) {
                            sendedCode = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);
                        }
                        verifyTimeout(startTime, endTime);

                    } while(bufferListened.type != ACKType || bufferListened.sourceAddress != clientAddress);
                    
                    sequence++;
                    message += 15;
                    messageSize -= 15;
                    cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

                } while (messageSize >= 1);
                
                sendedCode = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence++);
                time(&startTime);
                do {
                    receive = getMessage(socket, &bufferListened);
                    receive = getMessage(socket, &bufferListened);
                    
                    if(!checkParity(&bufferListened)) {
                        sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if(bufferListened.type == NACKType) {
                        printf("NACK, reenviando\n");
                        sendedCode = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence);
                    }
                    verifyTimeout(startTime, endTime);

                } while(bufferListened.type != ACKType || bufferListened.sourceAddress != clientAddress);
            }
        }

        if (bufferListened.type == showLinesBetweenType && bufferListened.sourceAddress == clientAddress) {
            int sequence = 0b0000;
            char *message = "";
            int sendedCode;
            int line;
            char fileName[256] = "";
            strcat(fileName, bufferListened.data);

            //ficar escutando o arquivo todo
            sendedCode = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessage(socket, &bufferListened);
                receive = getMessage(socket, &bufferListened);

                if(!checkParity(&bufferListened)) {
                    sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (bufferListened.type == NACKType && bufferListened.sourceAddress == clientAddress) {
                    printf("isNack, Tentando de novo\n");
                    sendedCode = sendMessage(socket, clientAddress, serverAddress, NACKType, message, sequence);
                } else if(bufferListened.type == showLinesBetweenType && bufferListened.sourceAddress == clientAddress) {
                    strcat(fileName, bufferListened.data);
                    sendedCode = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while(bufferListened.type != endTransmissionType || bufferListened.sourceAddress != clientAddress);

            sendedCode = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessage(socket, &bufferListened);
                receive = getMessage(socket, &bufferListened);

                if(!checkParity(&bufferListened)) {
                    sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (bufferListened.type == linesType && bufferListened.sourceAddress == clientAddress) {
                    line = atoi(bufferListened.data);
                }
                verifyTimeout(startTime, endTime);

            } while(bufferListened.type != linesType || bufferListened.sourceAddress != clientAddress);
            
            //pegar o conteúdo
            char initialLine[100];
            char finalLine[100];
            char *lines = strtok(bufferListened.data, " ");

            strcpy(initialLine, lines);
            lines = strtok(NULL, " ");
            strcpy(finalLine, lines);
            char fileContent[256] = "";

            int errorCode = showLinesContentInRangeServer(fileName, atoi(initialLine), atoi(finalLine), fileContent);
            if (errorCode != 0) {
                char errorCodeStr[4] = "";
                sprintf(errorCodeStr, "%d\n", errorCode);
                sendedCode = sendMessage(socket, clientAddress, serverAddress, ErrorType, errorCodeStr, sequence);
            } else {
                char *newMessage = calloc(15, sizeof(char));
                message = fileContent;
                int messageSize = strlen(message);
                int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;
                
                //ficar enviando o conteúdo
                do {
                    memset(newMessage, 0, 15);
                    memcpy(newMessage, message, cuttedMessageSize);
                    sendedCode = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);
                    time(&startTime);

                    do {
                        receive = getMessage(socket, &bufferListened);
                        receive = getMessage(socket, &bufferListened);

                        if(!checkParity(&bufferListened)) {
                            sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                        } else if(bufferListened.type == NACKType && bufferListened.sourceAddress == clientAddress) {
                            sendedCode = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);
                        }
                        verifyTimeout(startTime, endTime);

                    } while(bufferListened.type != ACKType || bufferListened.sourceAddress != clientAddress);
                    
                    sequence++;
                    message += 15;
                    messageSize -= 15;
                    cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

                } while (messageSize >= 1);
                
                sendedCode = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence++);
                time(&startTime);
                do {
                    receive = getMessage(socket, &bufferListened);
                    receive = getMessage(socket, &bufferListened);
                    
                    if(!checkParity(&bufferListened)) {
                        sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                    } else if(bufferListened.type == NACKType) {
                        printf("NACK, reenviando\n");
                        sendedCode = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence);
                    }
                    verifyTimeout(startTime, endTime);

                } while(bufferListened.type != ACKType || bufferListened.sourceAddress != clientAddress);
            }


        }

        if (bufferListened.type == editLineType && bufferListened.sourceAddress == clientAddress) {
            int sequence = 0b0000;
            char *message = "";
            int sendedCode;
            int line;
            char fileName[256] = "";
            strcat(fileName, bufferListened.data);

            //ficar escutando o arquivo todo
            sendedCode = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessage(socket, &bufferListened);
                receive = getMessage(socket, &bufferListened);

                if(!checkParity(&bufferListened)) {
                    sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (bufferListened.type == NACKType && bufferListened.sourceAddress == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    sendedCode = sendMessage(socket, clientAddress, serverAddress, NACKType, message, sequence);
                } else if(bufferListened.type == editLineType && bufferListened.sourceAddress == clientAddress) {
                    strcat(fileName, bufferListened.data);
                    sendedCode = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while(bufferListened.type != endTransmissionType || bufferListened.sourceAddress != clientAddress);
            
            sendedCode = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessage(socket, &bufferListened);
                receive = getMessage(socket, &bufferListened);

                if(!checkParity(&bufferListened)) {
                    sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (bufferListened.type == linesType && bufferListened.sourceAddress == clientAddress) {
                    line = atoi(bufferListened.data);
                }
                verifyTimeout(startTime, endTime);

            } while(bufferListened.type != linesType || bufferListened.sourceAddress != clientAddress);

            char newContent[256] = "";
            sendedCode = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            time(&startTime);
            do {
                receive = getMessage(socket, &bufferListened);
                receive = getMessage(socket, &bufferListened);

                if(!checkParity(&bufferListened)) {
                    sendedCode = sendMessage(socket, serverAddress, clientAddress, NACKType, message, sequence);
                } else if (bufferListened.type == NACKType && bufferListened.sourceAddress == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    sendedCode = sendMessage(socket, clientAddress, serverAddress, NACKType, message, sequence);
                } else if(bufferListened.type == fileContentType && bufferListened.sourceAddress == clientAddress) {
                    strcat(newContent, bufferListened.data);
                    sendedCode = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
                }
                verifyTimeout(startTime, endTime);

            } while(bufferListened.type != endTransmissionType || bufferListened.sourceAddress != clientAddress);

            int errorCode = editSpecificLineContent(fileName, line, newContent);
            if (errorCode != 0) {
                char errorCodeStr[4] = "";
                sprintf(errorCodeStr, "%d\n", errorCode);
                sendedCode = sendMessage(socket, clientAddress, serverAddress, ErrorType, errorCodeStr, sequence);
            } else {
                sendedCode = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            }

        }

    }

    return(0);
}