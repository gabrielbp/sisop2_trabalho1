#ifndef __functions__
#define __functions__

#include "cdata.h"

/**client**/
int clientSendFile(char *filename, int size, char *fileContent);
int clientDeleteFile(char* filename);
int clientDownloadFile(char *filename);
int clientListServer();

/**server**/
static void * serverUploadFile(void *arg);
static void * serverDeleteFile(void *arg, commandPacket commandPacket);
static void * serverDownloadFile(void *arg, commandPacket commandPacket);
static void * serverListServer(void *arg);
static void * exitCommand(void *arg);


#endif