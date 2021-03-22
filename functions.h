#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

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
    unsigned int start_marker: 8;
    unsigned int size: 4;
    unsigned int sequence: 4;
    unsigned int destination_address: 2;
    unsigned int source_address: 2;
    unsigned char data[256];
    unsigned int type: 4;
    unsigned int parity: 8;
} kermit_protocol_t;

kermit_protocol_t *defineProtocol(int destination_address, int source_address, int type, char *message, int sequence);

int getMessageFromAnotherProcess(int socket, kermit_protocol_t *received_buffer);

int sendMessage(int socket, int destinationAddres, int sourceAddres, int type, char* message, int sequence);

#endif