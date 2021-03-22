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

void verifyTimeout(time_t startTime, time_t endTime) {
    double timeDiff;

    endTime = time(NULL);
    timeDiff = difftime(endTime, startTime);
    if (timeDiff > 5) {
        printf("Timeout, exiting with code\n");
        exit(1);
    }
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

void calculateParity(kermit_protocol_t *buffer){
    int dataSize = buffer->size;
    unsigned int parity;
    parity = buffer->sequence ^ buffer->size ^ buffer->type;
    for (int i = 0; i < dataSize; i++) {
        parity = parity ^ buffer->data[i];
    }
    buffer->parity = parity;
}

int checkParity(kermit_protocol_t *buffer){
    int dataSize = buffer->size;
    unsigned int parity;
    parity = buffer->sequence ^ buffer->size ^ buffer->type;
    for (int i = 0; i < dataSize; i++) {
        parity = parity ^ buffer->data[i];
    }
    return(buffer->parity == parity);
}

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
    calculateParity(kermit);

    return kermit;
}

int receive, received_code;

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

int sendMessage(int socket, int destinationAddres, int sourceAddres, int type, char* message, int sequence) {
  kermit_protocol_t *send_buffer;
  send_buffer = defineProtocol(destinationAddres, sourceAddres, type, message, sequence);

  received_code = send(socket, send_buffer, sizeof(kermit_protocol_t), 0);
  free(send_buffer);

  if(received_code == -1) {
    exit(EXIT_SUCCESS);
  }

  return received_code;
}