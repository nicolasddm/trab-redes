#ifndef COMANDOS_H_
#define COMANDOS_H_

void listCurrentDirectoryFiles();

char changeDirectory(char *directory);

void showFileContentServer(char *file);

void showSpecificLineContentServer(char *file, int line);

void showLinesContentInRangeServer(char *file, int initialLine, int finalLine);

void editSpecificLineContent(char *file, int line, char *content);

#endif