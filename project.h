
#ifndef PROJECT_H
#define PROJECT_H

#define BUFF_SIZE               64
#define MAX_CLIENT_NUM          100

typedef enum {
    CLIENT_CMD_check=0,
    CLIENT_CMD_lock,
    CLIENT_CMD_unlock,
    CLIENT_CMD_clearalarm,
    CLIENT_CMD_video,
    CLIENT_CMD_quit,
    CLIENT_CMD_user,
    CLIENT_CMD_password,
    CLIENT_CMD_userretry,
    CLIENT_CMD_welcome,
    CLIENT_CMD_userexist,
    CLIENT_CMD_msg,
    CLIENT_CMD_none
}cmd_enum_t;

typedef struct pj_commands_s{
    cmd_enum_t cmd;
    char *cmd_str;
    int priv;
}pj_commands_t;

static pj_commands_t pjcmd[]={
    {.cmd=CLIENT_CMD_check,       .cmd_str="check"            , .priv=3},
    {.cmd=CLIENT_CMD_lock,        .cmd_str="lock"             , .priv=3},
    {.cmd=CLIENT_CMD_unlock,      .cmd_str="unlock"           , .priv=2},
    {.cmd=CLIENT_CMD_clearalarm,  .cmd_str="clearalarm"       , .priv=1},
    {.cmd=CLIENT_CMD_video,       .cmd_str="video"            , .priv=3},
    {.cmd=CLIENT_CMD_quit,        .cmd_str="quit"             , .priv=3},
    {.cmd=CLIENT_CMD_user,        .cmd_str="user:"            , .priv=3},
    {.cmd=CLIENT_CMD_password,    .cmd_str="password:"        , .priv=3},
    {.cmd=CLIENT_CMD_userretry,   .cmd_str="Please try again!", .priv=3},
    {.cmd=CLIENT_CMD_welcome,     .cmd_str="Welcome!"         , .priv=3},
    {.cmd=CLIENT_CMD_userexist,   .cmd_str="User exists!"     , .priv=3},
    {.cmd=CLIENT_CMD_msg,         .cmd_str="MSG:"             , .priv=3},
    {.cmd=CLIENT_CMD_none,        .cmd_str="none"             , .priv=3}
};

static int cmd_str_parse(char *rcv, int *cmd)
{
    int i=0;

    *cmd = CLIENT_CMD_none;
    while(pjcmd[i].cmd!=CLIENT_CMD_none){
        if(strncmp(pjcmd[i].cmd_str, rcv, strlen(pjcmd[i].cmd_str))==0)
        {
            *cmd = pjcmd[i].cmd;
            return 0;
        }
        i++;
    }
    return -1;
}

#endif //PROJECT_H
