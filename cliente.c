#include <dirent.h> 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
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


int main() {
    int socket = ConexaoRawSocket("lo");

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

            send(socket, command, 100, 0);
        }

        if(!strcmp(command, changeDirectoryServer)) {
            //codifica a mensagem
            //manda para o servidor
            //recebe resposta
            //decodifica
            //imprime resposta
        }

        if(!strcmp(command, showFileContent)) {
            //codifica a mensagem
            //manda para o servidor
            //recebe resposta
            //decodifica
            //imprime resposta
        }

        if(!strcmp(command, showSpecificLine)) {
            //codifica a mensagem
            //manda para o servidor
            //recebe resposta
            //decodifica
            //imprime resposta
        }

        if(!strcmp(command, showLinesBetween)) {
            //codifica a mensagem
            //manda para o servidor
            //recebe resposta
            //decodifica
            //imprime resposta
        }

        if(!strcmp(command, editLine)) {
            //codifica a mensagem
            //manda para o servidor
            //recebe resposta
            //decodifica
            //imprime resposta
        }

    }
    
    return(0);
}