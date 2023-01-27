#ifndef MYFTP_H
#define MYFTP_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define MY_PORT_NUMBER 49999
#define MAX_BUF_SIZE 4096
#define DEBUG 0

#endif

void getServResp(char *buffer, int socketfd); 
void parsePath(char *path, char *file);
int getDataConn(char *buffer, char *argv[], int socketfd, int socketfd2);
int isReg(const char *path);
int isDir(const char *path);
int listenConn();