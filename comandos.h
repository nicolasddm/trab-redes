#ifndef COMANDOS_H_
#define COMANDOS_H_

char listCurrentDirectoryFiles(char *directoryFilesList);

char changeDirectory(char *directory);

void showFileContentServer(char *file, char *fileContent);

void showSpecificLineContentServer(char *file, int line, char *fileContent);

void showLinesContentInRangeServer(char *file, int initialLine, int finalLine, char *fileContent);

void editSpecificLineContent(char *file, int line, char *content);

#endif