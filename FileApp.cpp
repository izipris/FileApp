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
#include <algorithm>

char* getIpFromSocket(int socketCode){
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int res = getpeername(socketCode, (struct sockaddr *)&addr, &addr_size);
    if (res > CODE_ERROR) {
        return inet_ntoa(addr.sin_addr);
    }
    exit(CODE_ERROR);
}

int isFileExist(char* fileName){
    int fd = open(fileName, O_RDONLY);
    if (fd > 0){
        close(fd);
        return 0;
    }
    close(fd);
    return CODE_ERROR;
}

int writeFileToSocket(int socketCode, const std::string& fileName){
    char tmp[fileName.size() + 1];
    strcpy(tmp, fileName.c_str());
    if (isFileExist(tmp) < 0){
        return CODE_ERROR;
    }
    std::ifstream file;
    file.open(fileName, std::ios_base::out | std::ios_base::binary);
    std::string contents((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();
    char fileData[contents.size() + 1];
    strcpy(fileData, contents.c_str());
    int len = strlen(fileData);
    long n = write(socketCode, &len, sizeof(int));
    if (n < 1){
        return CODE_ERROR;
    }
    n = write(socketCode, fileData, len);
    if (n < 1){
        return CODE_ERROR;
    }
    return 0;
}

int readFileFromSocket(int socketCode, char* data){
    int len;
    int n = read(socketCode, &len, sizeof(int));
    if (n < 1){
        return CODE_ERROR;
    }
    char fileData[len + 1];
    n = read(socketCode, fileData, len);
    fileData[len] = '\0';
    if (n < 1){
        return CODE_ERROR;
    }
    strcpy(data, fileData);
    return 0;
}

int writeErrorToSocket(int socketCode, const int errorCode){
    std::ofstream err;
    err.open("err.txt");
    err << errorCode;
    err.close();
    int code = writeFileToSocket(socketCode, "err.txt");
    remove("err.txt");
    return code;
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

void printClientSummary(char code){
    if (code == '0'){
        printf(SUCCESS_STR);
    } else if (code == '1'){
        printf(FILE_NAME_ERROR_STR);
        printf(FAILURE_STR);
        exit(CODE_ERROR);
    } else if (code == '2'){
        printf(MY_FILE_ERROR_STR);
        printf(FAILURE_STR);
        exit(CODE_ERROR);
    } else if (code == '3'){
        printf(REMOTE_FILE_ERROR_STR);
        printf(FAILURE_STR);
        exit(CODE_ERROR);
    }
}

void printServerMetadata(int connectionSocketCode, std::string dir, std::string& filePath, char& act){
    char metadata[FILE_CONTENT_LENGTH];
    readFileFromSocket(connectionSocketCode, metadata);
    char* chars_array = strtok(metadata, DELIMITER);
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
                filePath = dir;
                filePath.append("/");
                filePath.append(chars_array);
                std::cout << "file_path: " << filePath << "\n";
                break;
            default:
                exit(CODE_ERROR);
        }
        chars_array = strtok(NULL, DELIMITER);
    }
}


int main(int argc, char** argv) {
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
        std::string stdinWord;
        while (true){
            printf(WAIT_FOR_CLIENT_STR);
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(STDIN_FILENO, &readSet);
            FD_SET(serverEstablishmentCode, &readSet);
            if (select(serverEstablishmentCode + 1, &readSet, NULL, NULL, NULL) < 0){
                exit(CODE_ERROR);
            }
            if(FD_ISSET(STDIN_FILENO, &readSet)){
                std::cin >> stdinWord;
                if (stdinWord == QUIT){
                    close(serverEstablishmentCode);
                    return 0;
                }
            } else if ((connectionSocketCode = getConnection(serverEstablishmentCode)) != CODE_ERROR){
                char act;
                std::string filePath;
                printServerMetadata(connectionSocketCode, srv.directoryPath, filePath, act);
                // Do some checks
                bool passed = true;
                std::string dir = srv.directoryPath;
                size_t pathSlashNumber = std::count(filePath.begin(), filePath.end(), '/');
                size_t dirSlashNumber = std::count(dir.begin(), dir.end(), '/') + 1;
                if (filePath.size() >= FILE_NAME_LENGTH || (pathSlashNumber - dirSlashNumber > 0)){
                    passed = false;
                    printf(FILE_NAME_ERROR_STR);
                    writeErrorToSocket(connectionSocketCode, CODE_ERROR_FILENAME);
                }
                // Perform action
                if (passed && act == SERVER_U_MODE) {
                    // Read the content from the socket
                    char fileData[FILE_CONTENT_LENGTH];
                    readFileFromSocket(connectionSocketCode, fileData);
                    if (fileData[0] == '3'){
                        passed = false;
                        printf(REMOTE_FILE_ERROR_STR);
                        writeErrorToSocket(connectionSocketCode, CODE_ERROR_MYFILE);
                    } else {
                        // Create the file on the server
                        std::ofstream uploadedFile;
                        uploadedFile.open(filePath, std::ios_base::out | std::ios_base::binary);
                        uploadedFile << fileData;
                        uploadedFile.close();
                    }
                } else if (passed && act == SERVER_D_MODE){
                    // Send the desired file to the socket
                    int wr = writeFileToSocket(connectionSocketCode, filePath);
                    if (wr < 0){
                        writeErrorToSocket(connectionSocketCode, CODE_ERROR_REMOTE);
                        passed = false;
                        printf(MY_FILE_ERROR_STR);
                    }
                }
                if (passed){
                    writeErrorToSocket(connectionSocketCode, CODE_DONE);
                    printf(SUCCESS_STR);
                } else{
                    printf(FAILURE_STR);
                }
                close(connectionSocketCode);
            }
        }
    } else{
        char* command = argv[1];
        char* localPath = argv[2];
        char* remotePath = argv[3];
        int port = atoi(argv[4]);
        char* ipAddress = argv[5];
        int serverSocketCode = callSocket(ipAddress, port);
        if (serverSocketCode > 0){
            printf(CONNECTED_SUCCESSFULLY_STR);
        }
        // Write metadata of request to server
        std::ofstream metadata;
        metadata.open (MD_FILE_NAME);
        metadata << getIpFromSocket(serverSocketCode) << DELIMITER << command << DELIMITER <<
            localPath << DELIMITER << remotePath << DELIMITER;
        metadata.close();
        writeFileToSocket(serverSocketCode, MD_FILE_NAME);
        remove(MD_FILE_NAME);

        char code[2];
        if (strcmp(argv[1], MODE_UPLOAD) == 0){
            // Send the local file to the socket
            int wr = writeFileToSocket(serverSocketCode, localPath);
            if (wr < 0){
                writeErrorToSocket(serverSocketCode, CODE_ERROR_REMOTE);
            }
            readFileFromSocket(serverSocketCode, code);
        }
        else if (strcmp(argv[1], MODE_DOWNLOAD) == 0){
            // Read the file from the server
            char fileData[FILE_CONTENT_LENGTH];
            readFileFromSocket(serverSocketCode, fileData);
            if (fileData[0] == '3'){
                strcpy(code, "3");
            } else {
                // Create the local file
                std::string directory = localPath;
                directory.erase(directory.find_last_of('/')+1);
                std::string finalPath = "mkdir -p ";
                finalPath.append(directory);
                if ((system(finalPath.c_str())) < 0){
                    exit(CODE_ERROR);
                }
                std::ofstream downloadedFile(localPath);
                downloadedFile << fileData;
                downloadedFile.close();
                strcpy(code, "0");
            }
        }
        printClientSummary(code[0]);
        close(serverSocketCode);
    }
    return 0;
}