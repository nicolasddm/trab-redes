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

typedef struct {
    unsigned char command[100];
} cliente_t;


char device[] = "lo";
int received_code;

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
       

        if(!strcmp(command, changeDirectoryClient)) {
            char directory[100];
            scanf("%s", directory);
            changeDirectory(directory);
        }

        if(!strcmp(command, listCurrentDirectoryClient)){
            listCurrentDirectoryFiles();
        }
        
        if(!strcmp(command, listCurrentDirectoryServer)) {
            //codifica a mensagem
            //manda para o servidor
            //recebe resposta
            //decodifica
            //imprime resposta
            kermit_protocol_t *send_buffer;
            send_buffer = defineProtocol(serverAddress, clientAddress, listCurrentDirectoryType, command, 0);
            received_code = send(socket, send_buffer, sizeof(kermit_protocol_t), 0);
            printf("%d\n", received_code);
        }
        

        if(!strcmp(command, changeDirectoryServer)) {
            //codifica a mensagem
            //manda para o servidor
            //recebe resposta
            //decodifica
            //imprime resposta
            kermit_protocol_t *send_buffer;
            send_buffer = defineProtocol(serverAddress, clientAddress, changeDirectoryType, command, 0);
            received_code = send(socket, send_buffer, sizeof(kermit_protocol_t), 0);
            printf("%d\n", received_code);
        }

        if(!strcmp(command, showFileContent)) {
            //codifica a mensagem
            //manda para o servidor
            //recebe resposta
            //decodifica
            //imprime resposta
            kermit_protocol_t *send_buffer;
            send_buffer = defineProtocol(serverAddress, clientAddress, showFileContentType, command, 0);
            received_code = send(socket, send_buffer, sizeof(kermit_protocol_t), 0);
            printf("%d\n", received_code);
        }

        if(!strcmp(command, showSpecificLine)) {
            //codifica a mensagem
            //manda para o servidor
            //recebe resposta
            //decodifica
            //imprime resposta
            kermit_protocol_t *send_buffer;
            send_buffer = defineProtocol(serverAddress, clientAddress, showSpecificLineType, command, 0);
            received_code = send(socket, send_buffer, sizeof(kermit_protocol_t), 0);
            printf("%d\n", received_code);
        }

        if(!strcmp(command, showLinesBetween)) {
            //codifica a mensagem
            //manda para o servidor
            //recebe resposta
            //decodifica
            //imprime resposta
            kermit_protocol_t *send_buffer;
            send_buffer = defineProtocol(serverAddress, clientAddress, showLinesBetweenType, command, 0);
            received_code = send(socket, send_buffer, sizeof(kermit_protocol_t), 0);
            printf("%d\n", received_code);
        }

        if(!strcmp(command, editLine)) {
            //codifica a mensagem
            //manda para o servidor
            //recebe resposta
            //decodifica
            //imprime resposta
            kermit_protocol_t *send_buffer;
            send_buffer = defineProtocol(serverAddress, clientAddress, editLineType, command, 0);
            received_code = send(socket, send_buffer, sizeof(kermit_protocol_t), 0);
            printf("%d\n", received_code);
        }

    }
    
    return(0);
}