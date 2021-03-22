#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_
#include <time.h>

char changeDirectoryServer[100] = "cd";
char changeDirectoryClient[100] = "lcd";
char listCurrentDirectoryServer[100] = "ls";
char listCurrentDirectoryClient[100] = "lls";
char showFileContent[100] = "ver";
char showSpecificLine[100] = "linha";
char showLinesBetween[100] = "linhas";
char editLine[100] = "edit";

int clientAddress = 0b01;
int serverAddress = 0b10;
int changeDirectoryType = 0b0000;
int listCurrentDirectoryType = 0b0001;
int showFileContentType = 0b0010;
int showSpecificLineType = 0b0011;
int showLinesBetweenType = 0b0100;
int editLineType = 0b0101;
int ACKType = 0b1000;
int NACKType = 0b1001;
int linesType = 0b1010;
int listDirectoryContentType = 0b1011;
int fileContentType = 0b1100;
int endTransmissionType = 0b1101;
int ErrorType = 0b1111;

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

void calculateParity(kermit_type *buffer);

int checkParity(kermit_type *buffer);

kermit_type *mountBuffer(int destinationAddress, int sourceAddress, int type, char *message, int sequence);

void verifyTimeout(time_t startTime, time_t endTime);

int getMessage(int socket, kermit_type *bufferListened);

int sendMessage(int socket, int destinationAddres, int sourceAddress, int type, char* message, int sequence);

#endif