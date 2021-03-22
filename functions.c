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
#include <time.h>

typedef struct {
    unsigned int startMarker: 8;
    unsigned int size: 4;
    unsigned int sequence: 4;
    unsigned int destinationAddress: 2;
    unsigned int sourceAddress: 2;
    unsigned char data[256];
    unsigned int type: 4;
    unsigned int parity: 8;
} kermit_type;

void calculateParity(kermit_type *buffer){
    int dataSize = buffer->size;
    unsigned int parity;
    parity = buffer->sequence ^ buffer->size ^ buffer->type;
    for (int i = 0; i < dataSize; i++) {
        parity = parity ^ buffer->data[i];
    }
    buffer->parity = parity;
}

int checkParity(kermit_type *buffer){
    int dataSize = buffer->size;
    unsigned int parity;
    parity = buffer->sequence ^ buffer->size ^ buffer->type;
    for (int i = 0; i < dataSize; i++) {
        parity = parity ^ buffer->data[i];
    }
    return(buffer->parity == parity);
}

kermit_type *mountBuffer(
    int destinationAddress,
    int sourceAddress,
    int type,
    char *message,
    int sequence
){
    kermit_type *kermit = (kermit_type *)calloc(1,sizeof(kermit_type));
    kermit->startMarker = 0b01111110;
    kermit->destinationAddress = destinationAddress;
    kermit->sourceAddress = sourceAddress;
    strcpy(kermit->data, message);
    kermit->sequence = sequence;
    kermit->type = type;
    kermit->size = strlen(message);
    calculateParity(kermit);

    return kermit;
}

int receive, sendedCode;

int getMessage(int socket, kermit_type *bufferListened) {
    int sendedCode;
    sendedCode = recv(
        socket, 
        bufferListened,
        sizeof(kermit_type), 
        0
    );
    return sendedCode;
}

int sendMessage(int socket, int destinationAddres, int sourceAddress, int type, char* message, int sequence) {
  kermit_type *sendBuffer;
  sendBuffer = mountBuffer(destinationAddres, sourceAddress, type, message, sequence);

  sendedCode = send(socket, sendBuffer, sizeof(kermit_type), 0);
  free(sendBuffer);

  if(sendedCode == -1) {
    exit(EXIT_SUCCESS);
  }

  return sendedCode;
}

void verifyTimeout(time_t startTime, time_t endTime) {
    double timeDiff;

    endTime = time(NULL);
    timeDiff = difftime(endTime, startTime);
    if (timeDiff > 5) {
        printf("Timeout, exiting with code\n");
        exit(1);
    }
}