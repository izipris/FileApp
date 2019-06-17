//
// Created by idzipris on 6/17/2019.
//
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include "FileApp.h"

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

int main(int argc, char** argv) {
    std::string instanceMode = argv[1];
    Server srv;
    if (strcmp(argv[1], MODE_SERVER) == 0){
        std::cout << "Server Mode" << std::endl;
        srv = {argv[2], SERVER_MAX_SESSIONS};
        int serverEstablishmentCode = establishServer(atoi(argv[3]), &srv);
        if (serverEstablishmentCode < 0){
            exit(CODE_ERROR);
        }
        std::cout << "Server bind IP: " << inet_ntoa(*(&srv.socket.sin_addr)) << std::endl;
        int connectionSocketCode;
        while ((connectionSocketCode = getConnection(serverEstablishmentCode)) != CODE_ERROR) { // TODO: The 'quit' thing

        }
    } else if (strcmp(argv[1], MODE_UPLOAD) == 0){

    } else if (strcmp(argv[1], MODE_DOWNLOAD) == 0){

    }
    return 0;
}