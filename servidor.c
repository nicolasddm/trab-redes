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
    
    while (1) {
        
        //recebe mensagem
        receive = getMessageFromAnotherProcess(socket, &received_buffer);
        receive = getMessageFromAnotherProcess(socket, &received_buffer);

        //verifica qual comando
        //executa comando

        //todo
        if (received_buffer.type == changeDirectoryType) {
            printf("%s\n", received_buffer.data);
            int chdirReturn;
            chdirReturn = changeDirectory(received_buffer.data);
            printf("%d\n", chdirReturn);
            char *message;
            int sequence = 0;
            
            if (chdirReturn == 0) {
                printf("entrou aqui\n");
                printf("%s\n", getcwd(directoryPath, 100));
                received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            } else if (chdirReturn == -1) {
                printf("entrou erro\n");
                message = "2";
                received_code = sendMessage(socket, clientAddress, serverAddress, ErrorType, message, sequence);
            }
        }

        //done
        if (received_buffer.type == listCurrentDirectoryType) {
            int sequence = 0;
            char *emptyMessage = "";
            int received_code;
            
            char newMessage[15];
            char directoryFilesList[256] = "";
            memset(directoryFilesList, 0, 256);
            listCurrentDirectoryFiles(directoryFilesList);
            
            char *message = directoryFilesList;
            int messageSize = strlen(message);
            int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            do {
                memset(newMessage, 0, 15);
                memcpy(newMessage, message, cuttedMessageSize);

                received_code = sendMessage(socket, clientAddress, serverAddress, listDirectoryContentType, newMessage, sequence);

                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(received_buffer.type == NACKType && received_buffer.source_address == clientAddress) {
                        received_code = sendMessage(socket, clientAddress, serverAddress, listDirectoryContentType, newMessage, sequence);
                    } else if(received_buffer.type == ErrorType) {
                        printf("Error: ");
                        printf("%s\n", received_buffer.data);
                    }

                } while(received_buffer.type != ACKType || received_buffer.source_address != clientAddress);
                
                sequence++;
                message += 15;
                messageSize -= 15;
                cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            } while (messageSize >= 1);
            
            received_code = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence++);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                }

            } while(received_buffer.type != ACKType || received_buffer.source_address != clientAddress);
        }

        //done
        if (received_buffer.type == showFileContentType && received_buffer.source_address == clientAddress) {
            int sequence = 0;
            char *message = "";
            int received_code;
            char fileName[256] = "";
            strcat(fileName, received_buffer.data);

            //ficar escutando o arquivo todo
            received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    received_code = sendMessage(socket, clientAddress, serverAddress, NACKType, message, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                } else if(received_buffer.type == showFileContentType && received_buffer.source_address == clientAddress) {
                    printf("%s\n", received_buffer.data);
                    strcat(fileName, received_buffer.data);
                    received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
                }

            } while(received_buffer.type != endTransmissionType || received_buffer.source_address != clientAddress);

            
            //pegar o conteúdo
            char fileContent[256] = "";
            showFileContentServer(fileName, fileContent);

            char newMessage[15];
            message = fileContent;
            int messageSize = strlen(message);
            int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;
            
            //ficar enviando o conteúdo
            do {
                memset(newMessage, 0, 15);
                memcpy(newMessage, message, cuttedMessageSize);
                received_code = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);

                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(received_buffer.type == NACKType && received_buffer.source_address == clientAddress) {
                        received_code = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);
                    } else if(received_buffer.type == ErrorType) {
                        printf("Error: ");
                        printf("%s\n", received_buffer.data);
                    }

                } while(received_buffer.type != ACKType || received_buffer.source_address != clientAddress);
                
                sequence++;
                message += 15;
                messageSize -= 15;
                cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            } while (messageSize >= 1);
            
            received_code = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence++);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                }

            } while(received_buffer.type != ACKType || received_buffer.source_address != clientAddress);
                   
        }

        //done
        if (received_buffer.type == showSpecificLineType) {
            int sequence = 0;
            char *message = "";
            int received_code;
            int line;
            char fileName[256] = "";
            strcat(fileName, received_buffer.data);

            //ficar escutando o arquivo todo
            received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if (received_buffer.type == NACKType && received_buffer.source_address == serverAddress) {
                    printf("isNack, Tentando de novo\n");
                    received_code = sendMessage(socket, clientAddress, serverAddress, NACKType, message, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                } else if(received_buffer.type == showSpecificLineType && received_buffer.source_address == clientAddress) {
                    printf("%s\n", received_buffer.data);
                    strcat(fileName, received_buffer.data);
                    received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
                }

            } while(received_buffer.type != endTransmissionType || received_buffer.source_address != clientAddress);

            received_code = sendMessage(socket, clientAddress, serverAddress, ACKType, message, sequence);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                if (received_buffer.type == linesType && received_buffer.source_address == clientAddress) {
                    line = atoi(received_buffer.data);
                }

            } while(received_buffer.type != linesType || received_buffer.source_address != clientAddress);

            
            //pegar o conteúdo
            char fileContent[256] = "";
            showSpecificLineContentServer(fileName, line, fileContent);
            printf("%s\n", fileContent);
            
            char newMessage[15];
            message = fileContent;
            int messageSize = strlen(message);
            int cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;
            
            //ficar enviando o conteúdo
            do {
                memset(newMessage, 0, 15);
                memcpy(newMessage, message, cuttedMessageSize);
                received_code = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);

                do {
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);
                    receive = getMessageFromAnotherProcess(socket, &received_buffer);

                    if(received_buffer.type == NACKType && received_buffer.source_address == clientAddress) {
                        received_code = sendMessage(socket, clientAddress, serverAddress, fileContentType, newMessage, sequence);
                    } else if(received_buffer.type == ErrorType) {
                        printf("Error: ");
                        printf("%s\n", received_buffer.data);
                    }

                } while(received_buffer.type != ACKType || received_buffer.source_address != clientAddress);
                
                sequence++;
                message += 15;
                messageSize -= 15;
                cuttedMessageSize = messageSize >= 15 ? 15 : messageSize;

            } while (messageSize >= 1);
            
            received_code = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence++);
            do {
                receive = getMessageFromAnotherProcess(socket, &received_buffer);
                receive = getMessageFromAnotherProcess(socket, &received_buffer);

                if(received_buffer.type == NACKType) {
                    printf("NACK, reenviando\n");
                    received_code = sendMessage(socket, clientAddress, serverAddress, endTransmissionType, emptyMessage, sequence);
                } else if(received_buffer.type == ErrorType) {
                    printf("Error: ");
                    printf("%s\n", received_buffer.data);
                }

            } while(received_buffer.type != ACKType || received_buffer.source_address != clientAddress);
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