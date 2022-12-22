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


char rec[256];
int socket_msg_handle(int fd)
{
    int n;

    //memset((void *)rec, 0, sizeof(rec));
    n = recv(fd, rec, sizeof(rec),0);
    printf("%s\n",rec);
    if (strncmp("Meal completed and you need to pay",rec, strlen("Meal completed and you need to pay"))==0)
        return -1;
}

int user_cmd_handle(int fd)
{
    char userCmd[256];

    fgets(userCmd, 256, stdin);
    //printf("user input=%s",userCmd);
    if(strncmp(userCmd,"Cancel",6)==0)
        return -1;
    send(fd,userCmd, strlen(userCmd), 0);
    return 0;
}


int main(int argc, char *argv[])
{
    int sockFd;
    struct sockaddr_in server_sin;
    fd_set readfds;
    int nfds;

    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_sin, 0, sizeof(server_sin));
    server_sin.sin_family = AF_INET;
    server_sin.sin_port = htons(4444);
    inet_pton(AF_INET, "127.0.0.1", &server_sin.sin_addr.s_addr);

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