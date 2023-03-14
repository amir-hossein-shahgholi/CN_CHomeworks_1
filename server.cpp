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
std::string menu;
class User
{
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
class UserStatus
{
public:
    int user_id;
    bool is_login;
    int fd_id;
    int signup_state;
    User temp_info;
    bool menu_state;
};
std::vector<UserStatus> users_status;

void raise_error(int error_no, int fd)
{
    std::string error_msg = error_dict[error_no];
    if (fd == 0)
    {
        printf("%s\n", error_msg.c_str());
    }
    else
    {
        send(fd, error_msg.c_str(), error_msg.size(), 0);
    }
}

User find_user_by_fd(int fd)
{
    for (const auto &user_status : users_status)
    {
        if (user_status.fd_id == fd)
        {
            for (const auto &user : users)
            {
                if (user.id == user_status.user_id)
                {
                    return user;
                }
            }
        }
    }
    return null_user;
}

bool is_admin(int fd)
{
    User temp = find_user_by_fd(fd);
    if (temp.isAdmin == "true")
        return true;
    return false;
}

void send_user_information(int fd)
{
    std::string message = "";
    User user = find_user_by_fd(fd);

    message += "User ID: " + std::to_string(user.id) + "\n";
    message += "Username: " + user.username + "\n";
    message += "Password: " + user.password + "\n";
    message += "Purse: " + user.purse + "\n";
    message += "Is Admin: " + user.isAdmin + "\n";
    message += "Phone Number: " + user.phoneNumber + "\n";
    message += "Address: " + user.address + "\n\n";
    send(fd, message.c_str(), message.size(), 0);
}

void view_all_users(int fd)
{
    if (is_admin(fd))
    {
        std::string message = "";
        for (const auto &user : users)
        {
            message += "User ID: " + std::to_string(user.id) + "\n";
            message += "Username: " + user.username + "\n";
            message += "Purse: " + user.purse + "\n";
            message += "Is Admin: " + user.isAdmin + "\n";
            message += "Phone Number: " + user.phoneNumber + "\n";
            message += "Address: " + user.address + "\n\n";
        }

        send(fd, message.c_str(), message.size(), 0);
    }
    else
    {
        raise_error(403, fd);
    }
}

void handle_menu_commands(std::vector<std::string> values, int fd_id)
{
    if ((values[0][0] < '0') || (values[0][0] > '9') || (values[0].size() > 1) || (values.size() != 1))
        raise_error(503, fd_id);
    int num = values[0][0] - '0';
    switch (num)
    {
    case 0:
        break;
    case 1:
        send_user_information(fd_id);
        break;
    case 2:
        view_all_users(fd_id);
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;
    case 6:
        break;
    case 7:
        break;
    case 8:
        break;
    case 9:
        break;
    }
}

void init_values()
{
    error_dict[101] = "Err -> 101: The desired room was not found\n";
    error_dict[102] = "Err -> 102: Your reservation was not found\n";
    error_dict[104] = "Err -> 104: Successfully added.\n";
    error_dict[105] = "Err -> 105: Successfully modified.\n";
    error_dict[106] = "Err -> 106: Successfully deleted.\n";
    error_dict[108] = "Err -> 108: Your account balance is not enough\n";
    error_dict[109] = "Err -> 109: The room capacity is full\n";
    error_dict[110] = "Err -> 110: Successfully done.\n";
    error_dict[111] = "Err -> 111: This room already exists\n";
    error_dict[201] = "Err -> 201: User logged out successfully.\n";
    error_dict[230] = "Err -> 230: User logged in.\n";
    error_dict[231] = "Err -> 231: User successfully signed up.\n";
    error_dict[311] = "Err -> 311: User Signed up. Enter your password, purse, phone and address.\n";
    error_dict[312] = "Err -> 312: Information was changed successfully.\n";
    error_dict[401] = "Err -> 401: Invalid value!\n";
    error_dict[403] = "Err -> 403: Access denied!\n";
    error_dict[412] = "Err -> 412: Invalid capacity value!\n";
    error_dict[413] = "Err -> 413: successfully Leaving.\n";
    error_dict[430] = "Err -> 430: Invalid username or password.\n";
    error_dict[451] = "Err -> 451: User already existed!\n";
    error_dict[503] = "Err -> 503: Bad Sequence of commands.\n";
    null_user.id = -1;
    menu = "1. View user information\n2. view all users\n3. View rooms information\n4. Booking\n5. Canceling\n6. pass day\n7. Edit information\n8. Leaving room\n9. Rooms\n0. Logout\n";
}

void send_menu(int fd)
{
    send(fd, menu.c_str(), menu.size(), 0);
}

int last_user_id()
{
    int max_id = 0;
    for (const auto &user : users)
    {
        if (user.id > max_id)
            max_id = user.id;
    }
    return max_id;
}

int setupServer(int port, std::string addr)
{
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

int acceptClient(int server_fd)
{
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&address_len);

    return client_fd;
}

void read_config_file(std::string *addr, int *port)
{
    std::ifstream file("config.json");
    if (file.is_open())
    {
        std::string data, line;
        while (getline(file, line))
            data += line;
        file.close();
        std::string temp;
        nlohmann::json config = nlohmann::json::parse(data);
        *addr = config["hostName"];
        *port = config["commandChannelPort"];
    }
}

bool is_valid_date_time(std::string buff)
{
    std::regex pattern("^(\\d{2})-(\\d{2})-(\\d{4})");
    if (regex_match(buff, pattern))
    {
        return true;
    }
    raise_error(401, 0);
    return false;
}

void sign_in(std::string username, std::string password, int fd_id)
{
    write(0, "Signing request\n", 17);
    for (const auto &user : users)
    {
        if (user.username == username && user.password == password)
        {
            raise_error(230, fd_id);
            for (auto &user_status : users_status)
            {
                if (user_status.fd_id == fd_id)
                {
                    user_status.is_login = true;
                    user_status.user_id = user.id;
                    user_status.menu_state = true;
                }
                send_menu(fd_id);
                printf("User: %s logged in.\n", user.username.c_str());
            }
            return;
        }
    }
    raise_error(430, fd_id);
    return;
}

void create_users()
{
    std::ifstream jsonFile("UsersInfo.json");
    nlohmann::json j;
    jsonFile >> j;
    for (auto &_user : j["users"])
    {
        User user;
        user.id = _user["id"];
        user.username = _user["user"];
        user.password = _user["password"];
        user.isAdmin = _user["admin"];

        if (_user.find("purse") != _user.end())
        {
            user.purse = _user["purse"];
        }
        if (_user.find("phoneNumber") != _user.end())
        {
            user.phoneNumber = _user["phoneNumber"];
        }
        if (_user.find("address") != _user.end())
        {
            user.address = _user["address"];
        }

        users.push_back(user);
    }
    return;
}

User complete_user_signup(User &user, std::vector<std::string> values, int data_type)
{
    std::string data;
    std::string message = "";
    switch (data_type)
    {
    case 0: // PASSWORD
        data = values[0];
        user.password = data;
        break;
    case 1: // PURSE
        data = values[0];
        user.purse = data;
        break;
    case 2: // PHONE NUMBER
        data = values[0];
        user.phoneNumber = data;
        break;
    case 3: // ADDRESS
        for (const auto &str : values)
        {
            message += str + " ";
        }
        message.pop_back();
        data = message;
        user.address = data;
        break;
    default: // Invalid
        raise_error(503, 0);
    }
    return user;
}

void user_signup(std::string username, int fd)
{
    for (const auto &user : users)
    {
        if (user.username == username)
        {
            raise_error(451, fd);
            return;
        }
    }
    User new_user;
    raise_error(311, fd);
    new_user.username = username;
    for (auto &user_status : users_status)
    {
        if (user_status.fd_id == fd)
        {
            user_status.signup_state = 0;
            user_status.temp_info = new_user;
        }
    }
}

void save_user(User user)
{
    user.isAdmin = "false";
    users.push_back(user);
    raise_error(231, 0);
}

std::vector<std::string> command_serializer(char buffer[2048])
{
    std::vector<std::string> values;
    std::stringstream stream(buffer);
    std::string temp;
    while (stream >> temp)
    {
        values.push_back(temp);
    }
    return values;
}

std::string set_time()
{
    char buffer[2048] = {0};
    read(0, buffer, 2048);
    std::vector<std::string> values = command_serializer(buffer);
    if (values.size() != 2)
    {
        raise_error(503, 0);
        memset(buffer, 0, 2048);
        return " ";
    }
    if (values[0] != "setTime")
    {
        raise_error(503, 0);
        memset(buffer, 0, 2048);
        return " ";
    }
    if (is_valid_date_time(values[1]))
    {
        memset(buffer, 0, 2048);
        return values[1];
    }
    memset(buffer, 0, 2048);
    return " ";
}

void handle_commands(std::vector<std::string> values, int fd_id)
{
    for (auto &user_status : users_status)
    {
        if (user_status.fd_id == fd_id)
        {
            if (user_status.signup_state != -1)
            { // Signup states.
                if (values.size() > 1 && user_status.signup_state != 3)
                {
                    raise_error(503, fd_id);
                    user_status.signup_state = -1;
                    return;
                }
                complete_user_signup(user_status.temp_info, values, user_status.signup_state);
                user_status.signup_state++;
                if (user_status.signup_state == 4)
                {
                    user_status.temp_info.id = last_user_id() + 1;
                    user_status.temp_info.isAdmin = "false";
                    users.push_back(user_status.temp_info);
                    user_status.signup_state = -1;
                    user_status.is_login = true;
                    user_status.menu_state = true;
                    raise_error(231, fd_id);
                    send_menu(fd_id);
                }
                return;
            }
            else if (user_status.menu_state)
            { // Menu commands
                handle_menu_commands(values, fd_id);
            }
        }
    }

    if (values[0] == "signin")
    {
        if (values.size() == 3)
            sign_in(values[1], values[2], fd_id);
        else
            raise_error(430, fd_id);
    }
    else if (values[0] == "signup")
    {
        User new_user;
        if (values.size() == 2)
        {
            user_signup(values[1], fd_id);
        }
        else
            raise_error(503, fd_id);
    }
}

void add_new_user_status(int socket_id)
{
    UserStatus temp;
    temp.fd_id = socket_id;
    temp.signup_state = -1;
    temp.menu_state = false;
    temp.is_login = false;
    users_status.push_back(temp);
}

int main(int argc, char const *argv[])
{
    init_values();
    int server_fd, new_socket, max_sd;
    char buffer[2048] = {0};
    fd_set master_set, working_set;
    std::string addr;
    int port;
    std::string date_time;
    read_config_file(&addr, &port);
    create_users();
    printf("Please set start time with command 'setTime', Pay attention to the date format. (EX. -setTime 10-10-2010)\n");
    while (1)
    {
        date_time = set_time();
        if (date_time != " ")
        {
            break;
        }
    }
    server_fd = setupServer(port, addr);
    FD_ZERO(&master_set);
    max_sd = server_fd;
    FD_SET(server_fd, &master_set);
    write(1, "Server is running\n", 18);
    while (1)
    {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);

        for (int i = 0; i <= max_sd; i++)
        {
            if (FD_ISSET(i, &working_set))
            {

                if (i == server_fd)
                { // new clinet
                    new_socket = acceptClient(server_fd);
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                    add_new_user_status(new_socket);
                    printf("New client connected. fd = %d\n", new_socket);
                }

                else
                { // client sending msg
                    int bytes_received;
                    bytes_received = recv(i, buffer, 2048, 0);

                    if (bytes_received == 0)
                    { // EOF
                        printf("client fd = %d closed\n", i);
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }
                    std::vector<std::string> values = command_serializer(buffer);
                    handle_commands(values, i);
                    memset(buffer, 0, 2048);
                }
            }
        }
    }

    return 0;
}