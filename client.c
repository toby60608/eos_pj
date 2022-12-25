/*
 * Maple Chen 2022
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>
#include "project.h"

int sockFd;

int socket_msg_handle(int fd)
{
    char buffer[BUFF_SIZE];
    int cmd, ret, n;

    do{
        memset((void *)buffer, 0, sizeof(buffer));
        n = recv(fd, buffer, sizeof(buffer),0);
        ret = cmd_str_parse(buffer, &cmd);
        if(cmd==CLIENT_CMD_quit)
        {
            return -1;
        }
    }while(ret!=0);

    switch(cmd){
        case CLIENT_CMD_userexist:
            printf("User exist. ");
        case CLIENT_CMD_userretry:
            printf("Please try again!\n");
        case CLIENT_CMD_user:
            printf("user:");
            memset((void *)buffer, 0, sizeof(buffer));
            fgets(buffer, BUFF_SIZE, stdin);
            send(fd, buffer, strlen(buffer), 0);
            break;
        case CLIENT_CMD_password:
            printf("password:");
            memset((void *)buffer, 0, sizeof(buffer));
            fgets(buffer, BUFF_SIZE, stdin);
            send(fd, buffer, strlen(buffer), 0);
            break;
        case CLIENT_CMD_welcome:
            printf("%s\n",pjcmd[CLIENT_CMD_welcome].cmd_str);
            break;
        case CLIENT_CMD_unlockpasscode:
            printf("%s",pjcmd[CLIENT_CMD_unlockpasscode].cmd_str);
            memset((void *)buffer, 0, sizeof(buffer));
            fgets(buffer, BUFF_SIZE, stdin);
            send(fd, buffer, strlen(buffer), 0);
            break;
            break;
        case CLIENT_CMD_msg:
            printf("%s\n",buffer);
            break;
        case CLIENT_CMD_none:
        default:
            break;
    }
    return 0;
}

int user_cmd_handle(int fd)
{
    char userCmd[BUFF_SIZE];
    int cmd, ret;

    do{
        fgets(userCmd, BUFF_SIZE, stdin);

        ret = cmd_str_parse(userCmd, &cmd);
        if(cmd==CLIENT_CMD_quit)
        {
            return -1;
        }
    }while(ret!=0);

    switch(cmd){
        case CLIENT_CMD_check:
            send(fd, userCmd, strlen(userCmd), 0);
            break;
        case CLIENT_CMD_lock:
            send(fd, userCmd, strlen(userCmd), 0);
            break;
        case CLIENT_CMD_unlock:
            send(fd, userCmd, strlen(userCmd), 0);
            break;
        case CLIENT_CMD_clearalarm:
            send(fd, userCmd, strlen(userCmd), 0);
            break;
        case CLIENT_CMD_video:
            send(fd, userCmd, strlen(userCmd), 0);
            break;
        case CLIENT_CMD_quit:
            break;
        case CLIENT_CMD_none:
        default:
            break;
    }

    return 0;
}


int main(int argc, char *argv[])
{
    struct sockaddr_in server_sin;
    fd_set readfds;
    int nfds;
    int user_input_i,i;
    char *user_input_s;
    int user_port;
    char *user_ip;

    if (argc != 3)
    {
        printf("ERROR: Need two and only two parameter!\n");
        return -1;
    }
    else
    {
        user_ip = argv[1];
        user_port = atoi(argv[2]);

    }

    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_sin, 0, sizeof(server_sin));
    server_sin.sin_family = AF_INET;
    server_sin.sin_port = htons(user_port);
    inet_pton(AF_INET, user_ip, &server_sin.sin_addr.s_addr);

    connect(sockFd, (struct sockaddr *)&server_sin, sizeof(server_sin));


    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(sockFd, &readfds);
        FD_SET(STDOUT_FILENO, &readfds);
        nfds = (sockFd>STDOUT_FILENO)?sockFd:STDOUT_FILENO;
        select(nfds+1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(STDOUT_FILENO, &readfds))
        {
            //printf("select stdin!\n");
            if (user_cmd_handle(sockFd) == -1)
                break;
        }
        if (FD_ISSET(sockFd, &readfds))
        {
            //printf("select socket!\n");
            if (socket_msg_handle(sockFd) == -1)
                break;
        }
    }

    close(sockFd);
    return 0;
}