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
#include "prints.h"

char* getIpFromSocket(int socketCode){
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int res = getpeername(socketCode, (struct sockaddr *)&addr, &addr_size);
    return inet_ntoa(addr.sin_addr);
}

int writeFileToSocket(int socketCode, std::string fileName){
    std::ifstream file(fileName);
    std::string contents((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();
    char fileData[contents.size() + 1];
    strcpy(fileData, contents.c_str());
    int len = strlen(fileData);
    int n = write(socketCode, &len, sizeof(int));
    n = write(socketCode, fileData, len);
    return n;
}

int readFileFromSocket(int socketCode, char* data){
    int len;
    int n = read(socketCode, &len, sizeof(int));
    char fileData[len + 1];
    n = read(socketCode, fileData, len);
    strcpy(data, fileData);
    return n;
}

int establishServer(unsigned short port, Server* srv){
    char serverName[MAXHOSTNAME+1];
    int socketCreationCode;
    struct sockaddr_in socketAddress;
    struct hostent *localhost;
    gethostname(serverName, MAXHOSTNAME);
    localhost = gethostbyname(serverName);
    if (localhost == NULL){
        return CODE_ERROR;
    }
    memset(&socketAddress, 0, sizeof(struct sockaddr_in));
    socketAddress.sin_family = localhost->h_addrtype;
    memcpy(&socketAddress.sin_addr, localhost->h_addr_list[0], localhost->h_length);
    socketAddress.sin_port = htons(port);
    srv->socket = socketAddress;
    socketCreationCode = socket(AF_INET, SOCK_STREAM, 0);
    if (socketCreationCode < 0){
        return CODE_ERROR;
    }
    int socketBindCode = bind(socketCreationCode, (struct sockaddr*)&socketAddress, sizeof(struct sockaddr_in));
    if (socketBindCode < 0){
        close(socketCreationCode);
        return CODE_ERROR;
    }
    listen(socketCreationCode, srv->maxConnections);
    return socketCreationCode;
}

int getConnection(int serverSocketCode){
    printf(WAIT_FOR_CLIENT_STR);
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
    int socketCreationCode = socket(socketAddress.sin_family, SOCK_STREAM, 0);
    if (socketCreationCode < 0){
        return CODE_ERROR;
    }
    int socketConnectionCode = connect(socketCreationCode, (struct sockaddr*)&socketAddress, sizeof(socketAddress));
    if (socketConnectionCode < 0){
        close(socketConnectionCode);
        return CODE_ERROR;
    }
    return socketCreationCode;
}



int main(int argc, char** argv) {
    std::string instanceMode = argv[1];
    Server srv;
    if (strcmp(argv[1], MODE_SERVER) == 0){
        srv.directoryPath = argv[2];
        srv.maxConnections = SERVER_MAX_SESSIONS;
        int serverEstablishmentCode = establishServer(atoi(argv[3]), &srv);
        if (serverEstablishmentCode < 0){
            exit(CODE_ERROR);
        }
        printf(SERVERS_BIND_IP_STR, inet_ntoa(*(&srv.socket.sin_addr)));
        mkdir(srv.directoryPath, ACCESSPERMS);
        int connectionSocketCode;
        while ((connectionSocketCode = getConnection(serverEstablishmentCode)) != CODE_ERROR) { // TODO: The 'quit' thing
            //Read & Print metadata
            char metadata[FILE_CONTENT_LENGTH];
            readFileFromSocket(connectionSocketCode, metadata);
            char act;
            std::string filePath;
            char* chars_array = strtok(metadata, "#");
            for (int i = 0; i < 4; ++i){
                switch (i){
                    case 0:
                        printf(CLIENT_IP_STR, chars_array);
                        break;
                    case 1:
                        act = chars_array[1];
                        printf(CLIENT_COMMAND_STR, act);
                        break;
                    case 2:
                        printf(FILENAME_STR, chars_array);
                        break;
                    case 3:
                        filePath = srv.directoryPath;
                        filePath.append("/");
                        filePath.append(chars_array);
                        std::cout << "file_path: " << filePath << "\n";
                        break;
                    default:
                        exit(CODE_ERROR);
                }
                chars_array = strtok(NULL, "#");
            }
            // Perform action
            if (act == 'u') {

                char fileData[FILE_CONTENT_LENGTH];
                readFileFromSocket(connectionSocketCode, fileData);
                std::ofstream uploadedFile;
                uploadedFile.open(filePath);
                uploadedFile << fileData;
                uploadedFile.close();
            } else if (act == 'd'){
                // Send the desired file to the socket
                writeFileToSocket(connectionSocketCode, filePath);
            }
        }
    } else{

        char* command = argv[1];
        char* localPath = argv[2];
        char* remotePath = argv[3];
        int port = atoi(argv[4]);
        char* ipAddress = argv[5];
        int serverSocketCode = callSocket(ipAddress, port);
        // Write metadata of request to server
        std::ofstream metadata;
        metadata.open ("metadata.txt");
        metadata << getIpFromSocket(serverSocketCode) << "#" << command << "#" << localPath << "#" << remotePath << "#";
        metadata.close();
        writeFileToSocket(serverSocketCode, "metadata.txt");
        remove("metadata.txt");

        if (strcmp(argv[1], MODE_UPLOAD) == 0){
            // Send the local file to the socket
            writeFileToSocket(serverSocketCode, localPath);
        }
        else if (strcmp(argv[1], MODE_DOWNLOAD) == 0){
            // Read the file from the server
            char fileData[FILE_CONTENT_LENGTH];
            readFileFromSocket(serverSocketCode, fileData);
            // Create the local file
            std::ofstream downloadedFile;
            downloadedFile.open(localPath);
            downloadedFile << fileData;
            downloadedFile.close();
        }

    }
    return 0;
}