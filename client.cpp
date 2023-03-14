#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>

int connectServer(int port)
{
    int fd;
    struct sockaddr_in server_address;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    { // checking for errors
        printf("Error in connecting to server\n");
    }

    return fd;
}

int main(int argc, char const *argv[])
{
    int fd;
    char buff[1024] = {0};

    fd = connectServer(8000);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    int max_fd = (fd > STDIN_FILENO) ? fd : STDIN_FILENO;
    int bytes_received;
    char buffer[1024];
    while (1)
    {
        fd_set tmp_fds = read_fds;
        int ready_fds = select(max_fd + 1, &tmp_fds, NULL, NULL, NULL);
        if (ready_fds == -1)
        {
            perror("select");
            break;
        }

        if (FD_ISSET(fd, &tmp_fds))
        {
            bytes_received = recv(fd, buffer, 1024, 0);
            if (bytes_received <= 0)
            {
                printf("Connection closed.\n");
                break;
            }
            buffer[bytes_received] = '\0';
            printf("%s\n", buffer);
        }

        if (FD_ISSET(STDIN_FILENO, &tmp_fds))
        {
            fgets(buffer, 1024, stdin);
            int len = strlen(buffer) - 1;
            buffer[len] = '\0';
            send(fd, buffer, len, 0);
        }
    }
    close(fd);
    return 0;
}
