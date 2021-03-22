#ifndef COMANDOS_H_
#define COMANDOS_H_

int listCurrentDirectoryFiles(char *directoryFilesList);

int changeDirectory(char *directory);

int showFileContentServer(char *file, char *fileContent);

int showSpecificLineContentServer(char *file, int line, char *fileContent);

int showLinesContentInRangeServer(char *file, int initialLine, int finalLine, char *fileContent);

int editSpecificLineContent(char *file, int line, char *content);

#endif