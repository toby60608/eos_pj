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

int sockFd, resultFd;
pthread_mutex_t server_db_mutex;

#define CLIENT_CMD_Check        0x1
#define CLIENT_CMD_Lock         0x2
#define CLIENT_CMD_Unlock       0x3
#define CLIENT_CMD_ClearAlarm   0x4
#define CLIENT_CMD_Video        0x5
#define CLIENT_CMD_Quit         0x6
#define CLIENT_CMD_WRONG        0x99

#define MAX_CLIENT_NUM          100
typedef struct threadInfo_s{
    int         used;
    pthread_t   threadId;
    int         fd;
    int         account_type; /* 1: center, 2:user*/
    char        username[10];
    char        passwd[10];
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

void customer_add(clientInfo_t *cinfo)
{
    pthread_mutex_lock(&server_db_mutex);

    pthread_mutex_unlock(&server_db_mutex);
}


int cmd_str_parse(char *rcv, int *cmd, clientInfo_t *cinfo)
{
    if(strncmp("Check", rcv, strlen("Check"))==0)
    {
        *cmd = CLIENT_CMD_Check;
    }
    else if(strncmp("Lock", rcv, strlen("Lock"))==0)
    {
        *cmd = CLIENT_CMD_Lock;
    }
    else if(strncmp("Unlock", rcv, strlen("Unlock"))==0)
    {
        *cmd = CLIENT_CMD_Unlock;
    }
    else if(strncmp("Clear Alarm", rcv, strlen("Clear Alarm"))==0)
    {
        *cmd = CLIENT_CMD_ClearAlarm;
    }
    else if(strncmp("Video", rcv, strlen("Video"))==0)
    {
        *cmd = CLIENT_CMD_Video;
    }
    else if(strncmp("Quit", rcv, strlen("Quit"))==0)
    {
        *cmd = CLIENT_CMD_Quit;
    }
    else
    {
        *cmd = CLIENT_CMD_WRONG;
        return -1;
    }

    return 0;
}


int user_cmd_handle(clientInfo_t *cinfo)
{
    int cmd;
    char rec[256];

    while(1)
    {
        memset(rec, 0, sizeof(rec));
        recv(cinfo->fd, rec, sizeof(rec),0);
        cmd_str_parse(rec,&cmd,cinfo);
        //printf("GOT: cmd=%d, meal_ID=%d, quantity=%d\n",cmd,meal_ID,quantity);
        switch (cmd){

            case CLIENT_CMD_Check:
                break;

            case CLIENT_CMD_Lock:
                break;

            case CLIENT_CMD_Unlock:
                break;

            case CLIENT_CMD_ClearAlarm:
                break;

            case CLIENT_CMD_Video:
                break;

            case CLIENT_CMD_Quit:
                return 0;

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

int account_check(clientInfo_t *cinfo)
{
    char rec[256];
    send(cinfo->fd, "user:",strlen("user:"), 0);
    recv(cinfo->fd, rec, sizeof(rec),0);

    memset((void *)rec,0,sizeof(rec));
    if(strncmp("center", rec, strlen("center"))==0)
    {
        strncpy(cinfo->username, "center", strlen("center"));
        cinfo->account_type = 1;
    }
    else if( (strncmp("user1", rec, strlen("user1"))==0)||
             (strncmp("user2", rec, strlen("user2"))==0) ||
             (strncmp("user3", rec, strlen("user3"))==0)
            )
    {
        strncpy(cinfo->username, rec, strlen(rec));
        cinfo->account_type = 2;
    }
    else
    {
        return -1;
    }

    memset((void *)rec,0,sizeof(rec));
    send(cinfo->fd, "password:",strlen("password:"), 0);
    recv(cinfo->fd, rec, sizeof(rec),0);

    if(strncmp(cinfo->username, rec, strlen(cinfo->username))==0)
    {
        send(cinfo->fd, "welcome",strlen("welcome"), 0);
        return 0;
    }

    send(cinfo->fd, "Please try again",strlen("Please try again"), 0);
    return -1;

}

void* client_task(void* p) {
    clientInfo_t *info = (clientInfo_t *)p;

    while( account_check(info) == -1){}

    user_cmd_handle(info);
    info->used =0;
    close(info->fd);
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

    memset((void *)clientInfo,0,sizeof(clientInfo_t)*MAX_CLIENT_NUM);

    signal(SIGINT, server_sigint_handler);
    pthread_mutex_init(&server_db_mutex, 0);
    pj_server_create(user_input_i);
    pj_client_handle(sockFd);
    close(sockFd);

    return 0;
}