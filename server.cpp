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
#include <ctime>

std::string date_time;
fd_set master_set, working_set;
std::map<int, std::string> error_dict;
std::string menu;
std::string edit_rooms_menu;
std::string booking_message;
std::string canceling_message;
std::string leaving_message;
std::string edit_message;
std::string pass_day_message;
struct Reservator
{
    int id;
    int numOfBeds;
    std::string reserveDate;
    std::string checkoutDate;
};

struct Room
{
    std::string number;
    int status;
    int price;
    int maxCapacity;
    int capacity;
    std::vector<Reservator> reservators;
};
std::vector<Room> rooms;
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
    bool edit_room_state;
    bool pass_day_state;
    bool booking_status;
    bool canceling_status;
    bool leaving_status;
    bool edit_status;
};
std::vector<UserStatus> users_status;

std::tm parse_date(const std::string &date_string)
{
    std::tm date = {};
    std::stringstream ss(date_string);
    ss >> date.tm_mday;
    ss.ignore();
    ss >> date.tm_mon;
    ss.ignore();
    ss >> date.tm_year;
    date.tm_mon -= 1;
    date.tm_year -= 1900;
    return date;
}

void pass_day(int num_of_days)
{
    struct tm tmDate = {0};
    strptime(date_time.c_str(), "%d-%m-%Y", &tmDate);
    time_t utcTime = mktime(&tmDate);
    utcTime += num_of_days * 24 * 60 * 60;
    tmDate = *localtime(&utcTime);
    char dateBuffer[11];
    strftime(dateBuffer, sizeof(dateBuffer), "%d-%m-%Y", &tmDate);
    std::string updatedDateStr = dateBuffer;
    date_time = updatedDateStr;
}

bool is_numeric(std::string str)
{
    for (char c : str)
    {
        if (!isdigit(c))
            return false;
    }
    return true;
}

bool compare_dates(const std::string &date_string1, const std::string &date_string2)
{
    std::tm date1 = parse_date(date_string1);
    std::tm date2 = parse_date(date_string2);
    std::time_t time1 = std::mktime(&date1);
    std::time_t time2 = std::mktime(&date2);
    if (time1 == -1 || time2 == -1)
    {
        return false;
    }
    return (time1 <= time2);
}

void read_rooms_information()
{
    std::ifstream ifs("RoomsInfo.json");
    nlohmann::json j;
    ifs >> j;

    for (const auto &room : j.at("rooms"))
    {
        Room r;
        r.number = room.at("number").get<std::string>();
        r.status = room.at("status").get<int>();
        r.price = room.at("price").get<int>();
        r.maxCapacity = room.at("maxCapacity").get<int>();
        r.capacity = room.at("capacity").get<int>();

        for (const auto &user : room.at("users"))
        {
            Reservator u;
            u.id = user.at("id").get<int>();
            u.numOfBeds = user.at("numOfBeds").get<int>();
            u.reserveDate = user.at("reserveDate").get<std::string>();
            u.checkoutDate = user.at("checkoutDate").get<std::string>();
            r.reservators.push_back(u);
        }

        rooms.push_back(r);
    }
}

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
    pass_day_message = "Enter num of days to pass.\n";
    edit_rooms_menu = "Options: add, modify and remove\n";
    booking_message = "Enter num of room, people, reservation start date and end date.\n";
    leaving_message = "Enter num of room\n";
    canceling_message = "Enter num of room and beds to cancel.\n";
    edit_message = "Enter new password phone and address\n";
    menu = "1. View user information\n2. view all users\n3. View rooms information\n4. Booking\n5. Canceling\n6. pass day\n7. Edit information\n8. Leaving room\n9. Rooms\n0. Logout\n";
}

void send_booking_room(int fd)
{
    send(fd, booking_message.c_str(), booking_message.size(), 0);
}

void send_edit_user(int fd)
{
    send(fd, edit_message.c_str(), edit_message.size(), 0);
}

void send_edit_rooms_menu(int fd)
{
    send(fd, edit_rooms_menu.c_str(), edit_rooms_menu.size(), 0);
}

void send_canceling_room(int fd)
{
    send(fd, canceling_message.c_str(), canceling_message.size(), 0);
}

void send_leaving_room(int fd)
{
    send(fd, leaving_message.c_str(), leaving_message.size(), 0);
}

void send_menu(int fd)
{
    send(fd, menu.c_str(), menu.size(), 0);
}

void send_passday_message(int fd)
{
    send(fd, pass_day_message.c_str(), pass_day_message.size(), 0);
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
                printf("User: %s logged in.\n", user.username.c_str());
                send_menu(fd_id);
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

void add_new_user_status(int socket_id)
{
    UserStatus temp;
    temp.fd_id = socket_id;
    temp.signup_state = -1;
    temp.menu_state = false;
    temp.is_login = false;
    temp.edit_room_state = false;
    temp.pass_day_state = false;
    temp.booking_status = false;
    temp.canceling_status = false;
    temp.leaving_status = false;
    temp.edit_status = false;
    users_status.push_back(temp);
}

void send_just_rooms(int fd)
{
    std::string message = "";
    for (const auto &room : rooms)
    {
        message += "Number: " + room.number + "\n";
        message += "Status: " + std::to_string(room.status) + "\n";
        message += "Price: " + std::to_string(room.price) + "\n";
        message += "Max Capacity: " + std::to_string(room.maxCapacity) + "\n";
        message += "Capacity: " + std::to_string(room.capacity) + "\n\n";
    }

    send(fd, message.c_str(), message.size(), 0);
}

void logout(int fd)
{
    int index = 0;
    for (auto &user_status : users_status)
    {
        if (user_status.fd_id == fd)
        {
            users_status.erase(users_status.begin() + index);
        }
        index++;
    }
    printf("user fd = %d logged out\n", fd);
    raise_error(201, fd);
    close(fd);
    FD_CLR(fd, &master_set);
}

void edit_rooms_mode(int fd)
{
    if (is_admin(fd))
    {
        for (auto &user_status : users_status)
        {
            if (user_status.fd_id == fd)
            {
                user_status.edit_room_state = true;
                send_edit_rooms_menu(fd);
            }
        }
    }
    else
    {
        raise_error(403, fd);
    }
}

void send_rooms_with_detail(int fd)
{
    std::string message = "";
    for (const auto &room : rooms)
    {
        message += "Number: " + room.number + "\n";
        message += "Status: " + std::to_string(room.status) + "\n";
        message += "Price: " + std::to_string(room.price) + "\n";
        message += "Max Capacity: " + std::to_string(room.maxCapacity) + "\n";
        message += "Capacity: " + std::to_string(room.capacity) + "\n";
        if (room.maxCapacity != room.capacity)
        {
            message += "Users:\n";
            for (const auto &reservator : room.reservators)
            {
                if (compare_dates(reservator.reserveDate, date_time) && compare_dates(date_time, reservator.checkoutDate))
                {
                    message += "User ID: " + std::to_string(reservator.id) + "\n";
                    message += "Number of beds: " + std::to_string(reservator.numOfBeds) + "\n";
                    message += "Reservation date: " + reservator.reserveDate + "\n";
                    message += "Checkout date: " + reservator.checkoutDate + "\n\n";
                }
            }
        }
    }

    send(fd, message.c_str(), message.size(), 0);
}

void view_rooms_information(int fd)
{
    if (is_admin(fd))
    {
        send_rooms_with_detail(fd);
    }
    else
    {
        send_just_rooms(fd);
    }
}

void update_rooms_status(std::string old_date)
{
    for (auto &room : rooms)
    {
        for (const auto &reservator : room.reservators)
        {
            if (compare_dates(reservator.checkoutDate, date_time) && (compare_dates(old_date, reservator.checkoutDate) || old_date == reservator.checkoutDate))
            {
                room.capacity += reservator.numOfBeds;
            }
        }
    }
}

void pass_day(std::vector<std::string> values, int fd_id)
{
    if (is_admin(fd_id))
    {
        if (values.size() != 2 or values[0] != "passDay")
        {
            raise_error(503, fd_id);
        }
        else
        {
            if (is_numeric(values[1]))
            {
                pass_day(stoi(values[1]));
                printf("Updated time: %s\n", date_time.c_str());
            }
            else
            {
                raise_error(503, fd_id);
            }
        }
    }
    else
        raise_error(403, fd_id);
}

void pass_day_mode(int fd)
{
    if (is_admin(fd))
    {
        for (auto &user_status : users_status)
        {
            if (user_status.fd_id == fd)
            {
                user_status.pass_day_state = true;
                send_passday_message(fd);
            }
        }
    }
    else
    {
        raise_error(403, fd);
    }
}

void booking(int fd)
{
    for (auto &user_status : users_status)
    {
        if (user_status.fd_id == fd)
        {
            user_status.booking_status = true;
            send_booking_room(fd);
        }
    }
}

void canceling(int fd)
{
    for (auto &user_status : users_status)
    {
        if (user_status.fd_id == fd)
        {
            user_status.canceling_status = true;
            send_canceling_room(fd);
        }
    }
}

void leaving_room(int fd)
{
    for (auto &user_status : users_status)
    {
        if (user_status.fd_id == fd)
        {
            user_status.leaving_status = true;
            send_leaving_room(fd);
        }
    }
}

void edit_user(int fd)
{
    for (auto &user_status : users_status)
    {
        if (user_status.fd_id == fd)
        {
            user_status.edit_status = true;
            send_edit_user(fd);
        }
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
        logout(fd_id);
        break;
    case 1:
        send_user_information(fd_id);
        break;
    case 2:
        view_all_users(fd_id);
        break;
    case 3:
        view_rooms_information(fd_id);
        break;
    case 4:
        booking(fd_id);
        break;
    case 5:
        canceling(fd_id);
        break;
    case 6:
        pass_day_mode(fd_id);
        break;
    case 7:
        edit_user(fd_id);
        break;
    case 8:
        leaving_room(fd_id);
        break;
    case 9:
        edit_rooms_mode(fd_id);
        break;
    }
}

bool is_room_number_exist(std::string room_number)
{
    for (const auto &room : rooms)
        if (room.number == room_number)
            return true;
    return false;
}

Room room_by_id(std::string room_number)
{
    Room s;
    for (const auto &room : rooms)
        if (room.number == room_number)
            return room;
    return s;
}

void add_new_room(std::string room_num, std::string maxcap, std::string price, int fd)
{
    if (is_numeric(maxcap) && is_numeric(price))
    {
        Room room;
        room.number = room_num;
        room.maxCapacity = stoi(maxcap);
        room.price = stoi(price);
        room.capacity = 0;
        room.status = 0;
        rooms.push_back(room);
        raise_error(104, fd);
    }
    else
    {
        raise_error(503, fd);
    }
}

void modify_room(std::string room_num, std::string maxcap, std::string price, int fd)
{
    if (is_numeric(maxcap) && is_numeric(price))
    {
        for (auto &room : rooms)
            if (room.number == room_num)
            {
                if ((stoi(maxcap) < room.maxCapacity) && room.status == 1)
                {
                    raise_error(109, fd);
                }
                else
                {
                    room.maxCapacity = stoi(maxcap);
                    room.price = stoi(price);
                    raise_error(105, fd);
                }
            }
    }
    else
    {
        raise_error(503, fd);
    }
}

void remove_room(std::string room_num, int fd)
{
    int index = 0;
    for (auto &room : rooms)
    {
        if (room.number == room_num)
        {
            if (room.status == 1)
            {
                raise_error(109, fd);
            }
            else
            {
                rooms.erase(rooms.begin() + index);
                raise_error(106, fd);
            }
        }
        index++;
    }
}

void handle_edit_rooms_commands(std::vector<std::string> values, int fd_id)
{
    if (values[0] == "add")
    {
        if (values.size() != 4)
        {
            raise_error(503, fd_id);
            return;
        }
        if (is_room_number_exist(values[1]))
        {
            raise_error(111, fd_id);
            return;
        }
        else
        {
            add_new_room(values[1], values[2], values[3], fd_id);
            return;
        }
    }
    else if (values[0] == "modify")
    {
        if (values.size() != 4)
        {
            raise_error(503, fd_id);
            return;
        }
        if (is_room_number_exist(values[1]))
        {
            modify_room(values[1], values[2], values[3], fd_id);
            return;
        }
        else
        {
            raise_error(101, fd_id);
            return;
        }
    }
    else if (values[0] == "remove")
    {
        if (values.size() != 2)
        {
            raise_error(503, fd_id);
            return;
        }
        if (is_room_number_exist(values[1]))
        {
            remove_room(values[1], fd_id);
            return;
        }
        else
        {
            raise_error(101, fd_id);
            return;
        }
    }
    else
    {
        raise_error(503, fd_id);
    }
}

void handle_booking_state(std::vector<std::string> values, int fd_id)
{
    if ((values[0] == "book") && (is_valid_date_time((values[3]))) && (is_valid_date_time((values[4]))))
    {
        if (!compare_dates(values[3], values[4]))
            raise_error(101, fd_id);
        Room room;
        User user = find_user_by_fd(fd_id);
        if (values.size() != 5)
            raise_error(503, fd_id);
        if (is_room_number_exist(values[1]))
        {
            room = room_by_id(values[1]);
            if (room.status == 1)
                raise_error(109, fd_id);
            else
            {
                if (room.maxCapacity < stoi(values[2]))
                    raise_error(109, fd_id);
                else
                {
                    if ((stoi(values[2]) * room.price) > stoi(user.purse))
                    {
                        raise_error(108, fd_id);
                    }
                    else
                    {
                        int flag = 0;
                        for (auto &reservator : room.reservators)
                        {
                            if (compare_dates(values[3], reservator.checkoutDate))
                            {
                                if (compare_dates(reservator.reserveDate, values[4]))
                                {
                                    if (room.maxCapacity - reservator.numOfBeds < stoi(values[2]))
                                        raise_error(109, fd_id);
                                    flag = 1;
                                }
                            }
                        }
                        if (flag == 0)
                        {
                            user.purse = std::to_string(stoi(user.purse) - (stoi(values[2]) * room.price));
                            Reservator res;
                            res.id = user.id;
                            res.numOfBeds = stoi(values[2]);
                            res.reserveDate = values[3];
                            res.checkoutDate = values[4];
                            room.reservators.push_back(res);
                        }
                    }
                }
            }
        }
        else
        {
            raise_error(101, fd_id);
        }
    }
    else
    {
        raise_error(503, fd_id);
    }
}

void handle_canceling_state(std::vector<std::string> values, int fd_id)
{
    if ((values[0] == "cancel"))
    {
        Room room;
        User user = find_user_by_fd(fd_id);
        if (values.size() != 3)
            raise_error(503, fd_id);
        if (is_room_number_exist(values[1]))
        {
            room = room_by_id(values[1]);
            if (room.maxCapacity < stoi(values[2]))
            {
                raise_error(102, fd_id);
            }
            else
            {
                int flag = 0;
                int count = 0;
                for (auto &reservator : room.reservators)
                {
                    if (user.id == reservator.id)
                    {
                        flag = 1;
                        if (reservator.numOfBeds < stoi(values[2]))
                        {
                            raise_error(102, fd_id);
                        }
                        else
                        {
                            if (compare_dates(reservator.reserveDate, date_time))
                            {
                                raise_error(102, fd_id);
                            }
                            else
                            {
                                user.purse += room.price * stoi(values[2]) / 2;
                                if (reservator.numOfBeds > stoi(values[2]))
                                {
                                    reservator.numOfBeds -= stoi(values[2]);
                                }
                                else
                                {
                                    room.reservators.erase(room.reservators.begin() + count);
                                }
                                raise_error(110, fd_id);
                            }
                        }
                    }
                    count++;
                }
                if (flag == 0)
                {
                    raise_error(101, fd_id);
                }
            }
        }
        else
        {
            raise_error(101, fd_id);
        }
    }
    else
    {
        raise_error(503, fd_id);
    }
}

void handle_leaving_state(std::vector<std::string> values, int fd_id)
{
    if ((values[0] == "room"))
    {
        Room room;
        User user = find_user_by_fd(fd_id);
        if (values.size() != 2)
            raise_error(503, fd_id);
        if (is_room_number_exist(values[1]))
        {
            room = room_by_id(values[1]);
            int flag = 0;
            int count = 0;
            for (auto &reservator : room.reservators)
            {
                if (user.id == reservator.id)
                {
                    flag = 1;
                    if (compare_dates(date_time, reservator.reserveDate))
                    {
                        raise_error(102, fd_id);
                    }
                    else
                    {
                        room.reservators.erase(room.reservators.begin() + count);
                        raise_error(110, fd_id);
                    }
                }
                count++;
            }
            if (flag == 0)
            {
                raise_error(102, fd_id);
            }
        }
        else
        {
            raise_error(101, fd_id);
        }
    }
    else
    {
        raise_error(503, fd_id);
    }
}

void handle_edit_state(std::vector<std::string> values, int fd_id)
{
    User user = find_user_by_fd(fd_id);
    if (values.size() != 3)
        raise_error(503, fd_id);
    user.password = values[0];
    user.phoneNumber = values[1];
    user.address = values[2];
    raise_error(312, fd_id);
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
                    send_menu(fd_id);
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
                }
                send_menu(fd_id);
                return;
            }
            else if (user_status.booking_status)
            { // booking
                handle_booking_state(values, fd_id);
                user_status.booking_status = false;
            }
            else if (user_status.edit_status)
            { // booking
                handle_edit_state(values, fd_id);
                user_status.edit_status = false;
            }
            else if (user_status.leaving_status)
            { // leaving
                handle_leaving_state(values, fd_id);
                user_status.leaving_status = false;
            }
            else if (user_status.canceling_status)
            { // canceling
                handle_canceling_state(values, fd_id);
                user_status.canceling_status = false;
            }
            else if (user_status.pass_day_state)
            {
                std::string old_date;
                old_date = date_time;
                pass_day(values, fd_id);
                update_rooms_status(old_date);
                user_status.pass_day_state = false;
            }

            else if (user_status.edit_room_state)
            { // Edit rooms commands
                handle_edit_rooms_commands(values, fd_id);
                user_status.edit_room_state = false;
            }
            else if (user_status.menu_state)
            { // Menu commands
                handle_menu_commands(values, fd_id);
            }
            if ((!user_status.edit_room_state) && (!user_status.edit_status) && (!user_status.leaving_status) && (!user_status.canceling_status) && (!user_status.booking_status) && (!user_status.pass_day_state) && (user_status.signup_state == -1) && (user_status.is_login))
            {
                send_menu(fd_id);
            }
        }
    }

    if (values[0] == "signin")
    {
        if (values.size() == 3)
        {
            sign_in(values[1], values[2], fd_id);
        }
        else
        {
            raise_error(503, fd_id);
        }
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

int main(int argc, char const *argv[])
{
    init_values();
    int server_fd, new_socket, max_sd;
    char buffer[2048] = {0};
    std::string addr;
    int port;
    read_config_file(&addr, &port);
    create_users();
    read_rooms_information();
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
                { // new User
                    new_socket = acceptClient(server_fd);
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                    add_new_user_status(new_socket);
                    printf("New client connected. fd = %d\n", new_socket);
                }

                else
                { // User sending msg
                    int bytes_received;
                    bytes_received = recv(i, buffer, 2048, 0);

                    if (bytes_received == 0)
                    { // Connection closed
                        logout(i);
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