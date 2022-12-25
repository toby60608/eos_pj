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
#include <signal.h>
#include "project.h"

int sockFd, resultFd;
pthread_mutex_t server_db_mutex;


typedef struct security_s{
    int security;   /* 1:open, 0:close */
    int door;       /* 1:open, 0:close */
}security_t;

security_t securityDB;

typedef struct threadInfo_s{
    int         used;
    pthread_t   threadId;
    int         fd;
    int         account_id;
}clientInfo_t;
clientInfo_t clientInfo[MAX_CLIENT_NUM];

clientInfo_t *client_info_get(void)
{
    int i;
    for(i=0;i<MAX_CLIENT_NUM;i++){
        if(clientInfo[i].used == 0){
            clientInfo[i].used =1;
            return &clientInfo[i];
        }
    }

    return 0;
}

typedef struct account_s{
    char *username;
    int user_priv;  /* 1: high,     2: middle,      3: low  */
    int online;     /* 1: online,   0: offline              */
}account_t;

account_t accounts[]={
    {.username = "center", .user_priv = 1, .online = 0},
    {.username = "user1",  .user_priv = 2, .online = 0},
    {.username = "user2",  .user_priv = 2, .online = 0},
    {.username = "user3",  .user_priv = 3, .online = 0},
    {.username = "NONE"}
};

int unlock_handler(clientInfo_t *cinfo)
{
    char buf[BUFF_SIZE],*str;
    int passcode = rand();
    int user_passcode;

    srand (time(NULL));
    passcode = rand();
    send(cinfo->fd, pjcmd[CLIENT_CMD_unlockpasscode].cmd_str,strlen(pjcmd[CLIENT_CMD_unlockpasscode].cmd_str), 0);
    printf("passcode=%d\n",passcode);
    memset((void *)buf,0,sizeof(buf));
    recv(cinfo->fd, buf, sizeof(buf),0);
    user_passcode = atoi(buf);

    if(user_passcode==passcode)
    {
        str="MSG:security is unlocked";
        printf("%s\n",str);
        send(cinfo->fd, str,strlen(str), 0);
        return 0;
    }
    else
    {
        str="MSG:security is NOT unlocked";
        printf("%s\n",str);
        send(cinfo->fd, str,strlen(str), 0);
        return -1;
    }
}

int account_check(clientInfo_t *cinfo)
{
    int i;
    char rec[BUFF_SIZE];

    send(cinfo->fd, pjcmd[CLIENT_CMD_user].cmd_str,strlen(pjcmd[CLIENT_CMD_user].cmd_str), 0);
    memset((void *)rec,0,sizeof(rec));
    recv(cinfo->fd, rec, sizeof(rec),0);
    i=0;
    do{
        if(strncmp(accounts[i].username, rec, strlen(accounts[i].username))==0)
        {
            if( accounts[i].online == 1)
            {
                send(cinfo->fd, pjcmd[CLIENT_CMD_userexist].cmd_str,strlen(pjcmd[CLIENT_CMD_userexist].cmd_str), 0);
                return -1;
            }else{
                cinfo->account_id = i;
                goto password_check;
            }
        }
        i++;
    }while(strncmp(accounts[i].username, "NONE", strlen(accounts[i].username)!=0));
    return -1;

password_check:
    memset((void *)rec,0,sizeof(rec));
    send(cinfo->fd, pjcmd[CLIENT_CMD_password].cmd_str,strlen(pjcmd[CLIENT_CMD_password].cmd_str), 0);
    memset((void *)rec,0,sizeof(rec));
    recv(cinfo->fd, rec, sizeof(rec),0);
    if(strncmp(accounts[cinfo->account_id].username, rec, strlen(accounts[cinfo->account_id].username))==0)
    {
        accounts[cinfo->account_id].online = 1;
        send(cinfo->fd, pjcmd[CLIENT_CMD_welcome].cmd_str,strlen(pjcmd[CLIENT_CMD_welcome].cmd_str), 0);
        return 0;
    }
    send(cinfo->fd, pjcmd[CLIENT_CMD_userretry].cmd_str,strlen(pjcmd[CLIENT_CMD_userretry].cmd_str), 0);
    return -1;

}


void customer_add(clientInfo_t *cinfo)
{
    pthread_mutex_lock(&server_db_mutex);

    pthread_mutex_unlock(&server_db_mutex);
}

int priv_check(int cmd, clientInfo_t *cinfo)
{
    char *str;

    if(pjcmd[cmd].priv >= accounts[cinfo->account_id].user_priv)
        return 0;

    str="MSG: no privilege";
    send(cinfo->fd, str,strlen(str), 0);
    return -1;
}

int user_cmd_handle(clientInfo_t *cinfo)
{
    int cmd;
    char buff[BUFF_SIZE], *str;

    while(1)
    {
        memset(buff, 0, sizeof(buff));
        if(recv(cinfo->fd, buff, sizeof(buff),0)==0)
            return 0;
        cmd_str_parse(buff,&cmd);
        if(priv_check(cmd,cinfo)==-1)
            continue;
        switch (cmd){
            case CLIENT_CMD_check:
                str="MSG:here is check result";
                send(cinfo->fd, str,strlen(str), 0);
                break;
            case CLIENT_CMD_lock:
                securityDB.security=1;
                str="MSG:door lock!";
                send(cinfo->fd, str,strlen(str), 0);
                break;
            case CLIENT_CMD_unlock:
                securityDB.security=0;
                unlock_handler(cinfo);
                break;
            case CLIENT_CMD_clearalarm:
                str="MSG:door alarm clear!";
                send(cinfo->fd, str,strlen(str), 0);
                break;
            case CLIENT_CMD_video:
                str="MSG:video is here";
                send(cinfo->fd, str,strlen(str), 0);
                break;
            case CLIENT_CMD_quit:
                return 0;
            case CLIENT_CMD_msg:
                printf("%s",buff);
                break;
            default:
                break;
        }//switch()
    }//while(1)

    return 0;
}

int pj_server_create(int port_no)
{
    struct sockaddr_in server_sin;

    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockFd == -1)
    {
        printf("socket failed!\n");
        return -1;
    }

    memset(&server_sin, 0, sizeof(server_sin));
    server_sin.sin_family = AF_INET;
    server_sin.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sin.sin_port = htons(port_no);
    if (bind(sockFd, (struct sockaddr *)&server_sin, sizeof(server_sin)) == -1)
    {
        printf("bind failed!\n");
        return -1;
    }

    if( listen(sockFd, 12) == -1)
    {
        printf("listen failed!\n");
        return -1;
    }

    return 0;
}


void server_sigint_handler(int sig)
{

    pthread_mutex_destroy(&server_db_mutex);
    close(sockFd);
    exit(0);
}


void* client_task(void* p) {
    clientInfo_t *cinfo = (clientInfo_t *)p;

    while( account_check(cinfo) == -1){}

    user_cmd_handle(cinfo);

    printf("%s leaves!\n",accounts[cinfo->account_id].username);
    sleep(1);
    cinfo->used =0;
    accounts[cinfo->account_id].online = 0;
    close(cinfo->fd);
    pthread_exit(NULL);
}

int pj_client_handle(int socket_fd)
{
    struct sockaddr acceptAddr;
    socklen_t acceptAddrlen;
    int acceptFd;

    while(1){
        clientInfo_t *client_info;
        acceptAddrlen = sizeof(acceptAddr);
        acceptFd = accept(socket_fd, &acceptAddr, &acceptAddrlen);
        if(acceptFd==-1)
        {
            printf("accept failed!\n");
            return -1;
        }
        client_info = client_info_get();

        if(client_info != 0)
        {
            client_info->fd=acceptFd;
            pthread_create(&client_info->threadId, NULL, client_task, (void *)client_info);
            pthread_detach(client_info->threadId);
        }
    }//while(1)

    return 0;
}


int main(int argc, char *argv[])
{
    int user_input_i,i;
    char *user_input_s;

    if (argc != 2)
    {
        printf("ERROR: Need one and only one parameter!\n");
        return -1;
    }
    else
    {
        user_input_s = argv[1];
        user_input_i = atoi(user_input_s);
        // printf("user_input_s=%s\n", user_input_s);
        // printf("user_input_i=%d\n", user_input_i);
    }

    securityDB.security =0;
    securityDB.door =0;

    memset((void *)clientInfo,0,sizeof(clientInfo_t)*MAX_CLIENT_NUM);

    signal(SIGINT, server_sigint_handler);
    pthread_mutex_init(&server_db_mutex, 0);
    pj_server_create(user_input_i);
    pj_client_handle(sockFd);
    close(sockFd);

    return 0;
}