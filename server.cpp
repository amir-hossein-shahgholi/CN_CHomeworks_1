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
#include <sstream>

std::map<int, std::string> error_dict;
void set_errors(){
    error_dict[101] = "Err -> 101: The desired room was not found ";
    error_dict[102] = "Err -> 102: Your reservation was not found";
    error_dict[104] = "Err -> 104: Successfully added.";
    error_dict[105] = "Err -> 105: Successfully modified.";
    error_dict[106] = "Err -> 106: Successfully deleted.";
    error_dict[108] = "Err -> 108: Your account balance is not enough ";
    error_dict[109] = "Err -> 109: The room capacity is full";
    error_dict[110] = "Err -> 110: Successfully done. ";
    error_dict[111] = "Err -> 111: This room already exists";
    error_dict[201] = "Err -> 201: User logged out successfully.";
    error_dict[230] = "Err -> 230: User logged in. ";
    error_dict[231] = "Err -> 231: User successfully signed up.";
    error_dict[311] = "Err -> 311: User Signed up. Enter your password, purse, phone and address.";
    error_dict[312] = "Err -> 312: Information was changed successfully.";
    error_dict[401] = "Err -> 401: Invalid value!";
    error_dict[403] = "Err -> 403: Access denied!";
    error_dict[412] = "Err -> 412: Invalid capacity value!";
    error_dict[413] = "Err -> 413: successfully Leaving.";
    error_dict[430] = "Err -> 430: Invalid username or password.";
    error_dict[451] = "Err -> 451: User already existed! ";
    error_dict[503] = "Err -> 503: Bad Sequence of commands.";
}

class User {
public:
    int id;
    std::string username;
    std::string password;
    std::string isAdmin;
    std::string purse;
    std::string phoneNumber;
    std::string address;
};
std::vector<User> users;
User null_user;

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

void raise_error(int error_no, int fd=0){
    std::string error_msg = error_dict[error_no];
    if (fd == 0){
        printf("%s\n", error_msg.c_str());
    }else{
        write(fd, error_msg.c_str(), error_msg.size());
    }
}

bool is_valid_date_time(std::string buff){
    std::regex pattern("^(\\d{2})-(\\d{2})-(\\d{4})");
    if (regex_match(buff, pattern)){
        return true;
    }
    raise_error(401);
    return false;
}

void print_users_info(){
    for (const auto& user : users) {
        std::cout << "ID: " << user.id << std::endl;
        std::cout << "Username: " << user.username << std::endl;
        std::cout << "Password: " << user.password << std::endl;
        std::cout << "Is admin: " << user.isAdmin << std::endl;
        std::cout << "Purse: " << user.purse << std::endl;
        std::cout << "Phone number: " << user.phoneNumber << std::endl;
        std::cout << "Address: " << user.address << std::endl;
        std::cout << std::endl;
    }
}

void sign_in(std::string username, std::string password){
    for (const auto& user : users) {
        if (user.username == username && user.password == password){
            raise_error(203);
            return;
        }
    }
    raise_error(430);
    return;
}

void create_users(){
    std::ifstream jsonFile("UsersInfo.json");
    nlohmann::json j;
    jsonFile >> j;
    for (auto& _user : j["users"]) {
        User user;
        user.id = _user["id"];
        user.username = _user["user"];
        user.password = _user["password"];
        user.isAdmin = _user["admin"];

        if (_user.find("purse") != _user.end()) {
            user.purse = _user["purse"];
        }
        if (_user.find("phoneNumber") != _user.end()) {
            user.phoneNumber = _user["phoneNumber"];
        }
        if (_user.find("address") != _user.end()) {
            user.address = _user["address"];
        }

        users.push_back(user);
    }
    return;
}

User complete_user_signup(User user, std::string data, int data_type){
    if (data.length() == 1){
        data_type = -1;
    }
    switch (data_type){
        case 0: //PASSWORD
            user.password = data;
            break;
        case 1: //PURSE
            user.purse = data;
            break;
        case 2: //PHONE NUMBER
            user.phoneNumber = data;
            break;
        case 3: // ADDRESS
            user.address = data;
            break;
        default: //Invalid
            raise_error(503);

    }
    return user;

}

User user_signup(std::string username){
    for (const auto& user : users) {
        if (user.username == username ){
            raise_error(451);
            return null_user;
        }
    }
    User new_user;
    raise_error(311);
    new_user.username = username;
    return new_user;
}

void save_user (User user){
    user.isAdmin = "false";
    users.push_back(user);
    raise_error(231);
}

std::vector<std::string> command_serializer(char buffer[2048]){
    std::vector<std::string> values;
    std::stringstream stream(buffer);
    std::string temp;
    while (stream >> temp) {
        values.push_back(temp);
    }
    return values;
}

std::string set_time(){
    char buffer[2048] = {0};
    read(0, buffer, 2048);
    std::vector<std::string> values = command_serializer(buffer);
    if (values.size() != 2){
        raise_error(503, 0);
        memset(buffer, 0, 2048);
        return " ";
    }
    if (values[0] != "setTime"){
        raise_error(503, 0);
        memset(buffer, 0, 2048);
        return " ";
    }
    if (is_valid_date_time(values[1])){
        memset(buffer, 0, 2048);
        return values[1];
    }
    memset(buffer, 0, 2048);
    return " ";
}

// void handle_commands(std::vector<std::string> values){
//     if (values[0] == "signin"){

//     }else if(values[0] == "signup"){

//     }
// }

int main(int argc, char const *argv[]) {
    set_errors();
    int server_fd, new_socket, max_sd;
    char buffer[2048] = {0};
    fd_set master_set, working_set;
    std::string addr;
    int port;
    std::string date_time;
    read_config_file(&addr, &port);
    create_users();
    printf("Please set start time with command 'setTime', Pay attention to the date format. (EX. -setTime 10-10-2010)\n");
    while (1){
        date_time = set_time();
        if(date_time != " "){
            break;
        }
    }
    server_fd = setupServer(port, addr);
    FD_ZERO(&master_set);
    max_sd = server_fd;
    FD_SET(server_fd, &master_set);
    write(1, "Server is running\n", 18);
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