#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fstream>
#include <iostream>
#include "req/json.h"
#include <cstring>
#include <regex>


int setupServer(int port, std::string addr) {
    struct sockaddr_in address;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(addr.c_str());
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    
    listen(server_fd, 4);

    return server_fd;
}

int acceptClient(int server_fd) {
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*) &address_len);

    return client_fd;
}

void read_config_file(std::string* addr, int* port){
    std::ifstream file("config.json");
    if (file.is_open()) {
        std::string data, line;
        while (getline(file, line)) data += line;
        file.close();
        std::string temp;
        nlohmann::json config = nlohmann::json::parse(data);
        *addr = config["hostName"];
        *port = config["commandChannelPort"];

    }
}

void raise_error(int error_no){
    printf("%d\n", error_no);
}

char* validate_date_time(char buff[1024]){
    if (strlen(buff) == 1){
        raise_error(503);
    }
    std::regex pattern("^(\\d{2})-(\\d{2})-(\\d{4})\n");
    if (regex_match(buff, pattern)){
        return buff;
    }
    raise_error(401);
    return {0};
}

int main(int argc, char const *argv[]) {
    int server_fd, new_socket, max_sd;
    char buffer[1024] = {0};
    fd_set master_set, working_set;
    std::string addr;
    int port;
    read_config_file(&addr, &port);
    server_fd = setupServer(port, addr);
    FD_ZERO(&master_set);
    max_sd = server_fd;
    FD_SET(server_fd, &master_set);
    write(1, "Server is running\n", 18);
    write(1, "Enter Date time(Ex. 25-02-2023)>>", 33);
    char buff[1024] = {0};
    read(0, buff, 1024);
    std::string date_time = validate_date_time(buff);
    std::cout<<date_time<<std::endl;
    while (1) {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);

        for (int i = 0; i <= max_sd; i++) {
            if (FD_ISSET(i, &working_set)) {
                
                if (i == server_fd) {  // new clinet
                    new_socket = acceptClient(server_fd);
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                    printf("New client connected. fd = %d\n", new_socket);
                }
                
                else { // client sending msg
                    int bytes_received;
                    bytes_received = recv(i , buffer, 1024, 0);
                    
                    if (bytes_received == 0) { // EOF
                        printf("client fd = %d closed\n", i);
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }

                    printf("client %d: %s\n", i, buffer);
                    memset(buffer, 0, 1024);
                }
            }
        }

    }

    return 0;
}