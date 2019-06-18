//
// Created by idzipris on 6/17/2019.
//
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <fcntl.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <sys/stat.h>
#include "FileApp.h"

long FdGetFileSize(int fd)
{
    struct stat stat_buf;
    int rc = fstat(fd, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

int readData(int fd, char *buf, long n) {
    int bcount; /* counts bytes read */
    int br; /* bytes read this pass */
    bcount= 0;
    br = 0;
    while (bcount < n) { /* loop until full buffer */
        br = read(fd, buf, n-bcount);
        if (br > 0) {
            std::cout << "Reading" << std::endl;
            bcount += br;
            buf += br;
        }
        if (br < 1) {
            return CODE_ERROR;
        }
    }
    return(bcount);
}

int writeData(int socketCode, char *name, int nameLen, char *content, int contentLen) {
    int n = write(socketCode,&nameLen,sizeof(int));
    n = write(socketCode,name,nameLen);
    n = write(socketCode,&contentLen,sizeof(int));
    n = write(socketCode,content,contentLen);
    return n;
}

int createFileFromSocket(int socketCode){
    char fileName[FILE_NAME_LENGTH + sizeof(int)];
    bzero(fileName,FILE_NAME_LENGTH + sizeof(int));
    int nameLen;
    int n = read(socketCode,&nameLen,sizeof(int));
    n = read(socketCode,fileName,nameLen);

    char fileData[FILE_CONTENT_LENGTH + sizeof(int)];
    bzero(fileData,FILE_CONTENT_LENGTH + sizeof(int));
    int contentLen;
    n = read(socketCode,&contentLen,sizeof(int));
    n = read(socketCode,fileData,contentLen);
    std::ofstream file;
    file.open (fileName);
    file << fileData;
    file.close();
    return n;
}

int establishServer(unsigned short port, Server* srv){
    char serverName[MAXHOSTNAME+1];
    int socketCreationCode;
    struct sockaddr_in socketAddress;
    struct hostent *localhost;
    gethostname(serverName, MAXHOSTNAME);
    std::cout << "Got host name" << std::endl;
    localhost = gethostbyname(serverName);
    std::cout << "Got host details" << std::endl;
    if (localhost == NULL){
        return CODE_ERROR;
    }
    memset(&socketAddress, 0, sizeof(struct sockaddr_in));
    socketAddress.sin_family = localhost->h_addrtype;
    memcpy(&socketAddress.sin_addr, localhost->h_addr_list[0], localhost->h_length);
    socketAddress.sin_port = htons(port);
    srv->socket = socketAddress;
    std::cout << "Initialized socket" << std::endl;
    socketCreationCode = socket(AF_INET, SOCK_STREAM, 0);
    std::cout << "Created socket" << std::endl;
    if (socketCreationCode < 0){
        return CODE_ERROR;
    }
    int socketBindCode = bind(socketCreationCode, (struct sockaddr*)&socketAddress, sizeof(struct sockaddr_in));
    std::cout << "Binded socket" << std::endl;
    if (socketBindCode < 0){
        close(socketCreationCode);
        return CODE_ERROR;
    }
    listen(socketCreationCode, srv->maxConnections);
    std::cout << "Listening..." << std::endl;
    return socketCreationCode;
}

int getConnection(int serverSocketCode){
    std::cout << "Waiting for a client..." << std::endl;
    int socketConnectionCode;
    socketConnectionCode = accept(serverSocketCode, NULL, NULL);
    if (socketConnectionCode < 0){
        return CODE_ERROR;
    }
    return socketConnectionCode;
}

int callSocket(char* ipAddress, int port){
    struct sockaddr_in socketAddress;
    memset(&socketAddress, 0, sizeof(socketAddress));
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_port = htons(port);
    inet_aton(ipAddress, &(socketAddress.sin_addr));
    std::cout << "Initialized socket" << std::endl;
    int socketCreationCode = socket(socketAddress.sin_family, SOCK_STREAM, 0);
    if (socketCreationCode < 0){
        return CODE_ERROR;
    }
    std::cout << "Reached Socket" << std::endl;
    int socketConnectionCode = connect(socketCreationCode, (struct sockaddr*)&socketAddress, sizeof(socketAddress));
    if (socketConnectionCode < 0){
        close(socketConnectionCode);
        return CODE_ERROR;
    }
    std::cout << "Connected Socket" << std::endl;
    return socketCreationCode;
}



int main(int argc, char** argv) {
    std::string instanceMode = argv[1];
    Server srv;
    if (strcmp(argv[1], MODE_SERVER) == 0){
        std::cout << "Server Mode" << std::endl;
        srv.directoryPath = argv[2];
        srv.maxConnections = SERVER_MAX_SESSIONS;
        int serverEstablishmentCode = establishServer(atoi(argv[3]), &srv);
        if (serverEstablishmentCode < 0){
            exit(CODE_ERROR);
        }
        std::cout << "Server bind IP: " << inet_ntoa(*(&srv.socket.sin_addr)) << std::endl;
        int connectionSocketCode;
        while ((connectionSocketCode = getConnection(serverEstablishmentCode)) != CODE_ERROR) { // TODO: The 'quit' thing
            std::cout << "Got Connection from: " << connectionSocketCode << std::endl;
            int rd = createFileFromSocket(connectionSocketCode);
        }
    } else if (strcmp(argv[1], MODE_UPLOAD) == 0){
        std::cout << "Upload Mode" << std::endl;
        char* localPath = argv[2];
        int localPathFd = open(localPath, O_RDONLY);
        char* remotePath = argv[3];
        int port = atoi(argv[4]);
        char* ipAddress = argv[5];
        int serverSocketCode = callSocket(ipAddress, port);
        char bufferName[FILE_NAME_LENGTH];
        char bufferContent[FILE_CONTENT_LENGTH];
        bzero(bufferName,FILE_NAME_LENGTH);
        bzero(bufferContent,FILE_CONTENT_LENGTH);
        memcpy(bufferName, remotePath, strlen(remotePath));
        read(localPathFd, bufferContent, FILE_CONTENT_LENGTH - 1);
        int wr = writeData(serverSocketCode, bufferName, strlen(bufferName), bufferContent, strlen(bufferContent));
        close(localPathFd);

    } else if (strcmp(argv[1], MODE_DOWNLOAD) == 0){

    }
    return 0;
}