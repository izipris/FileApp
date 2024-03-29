//
// Created by idzipris on 6/17/2019.
//
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#ifndef FILEAPP_FILEAPP_H
#define FILEAPP_FILEAPP_H

#define CODE_DONE 0
#define CODE_ERROR_FILENAME 1
#define CODE_ERROR_MYFILE 2
#define CODE_ERROR_REMOTE 3
#define CODE_ERROR -1
#define MODE_SERVER "-s"
#define MODE_UPLOAD "-u"
#define MODE_DOWNLOAD "-d"
#define SERVER_MAX_SESSIONS 3
#define MAXHOSTNAME 1000
#define FILE_CONTENT_LENGTH 4096
#define FILE_NAME_LENGTH 256
#define DELIMITER "#"
#define QUIT "quit"
#define MD_FILE_NAME "metadata.txt"
#define SERVER_D_MODE 'd'
#define SERVER_U_MODE 'u'

typedef struct { char* directoryPath; int maxConnections; sockaddr_in socket; } Server;

#endif //FILEAPP_FILEAPP_H
