#ifndef  _CONFIG_H
#define  _CONFIG_H

#include <stdio.h>
#include <stdlib.h>


#define THREEAD_MAX 4096
#define LISTEN_MAX 500
#define SERVER_IP "192.168.1.106"

#define LSERVER2WSERVER ".lserver2wserver"
#define WSERVER2LSERVER ".wserver2lserver"


typedef struct
{
	char ip4[128];
	int port;
	int fd;
}LISTEN_INFO;

typedef struct
{
    char *msg;         //send message
    int  size;         //message len
    int  sock_fd;      //sock fd
    int  read_fd;      //read fd
    int  write_fd;     //write fd
}SOCKDATA;

/** JSON消息结构
client ------------------------>web server----------------------------------->wow server
                {                                {
                    "type":1                        “needResponse” : 1,
                    "command": "xxxx"               "message": "xxxxx";
                }                                   "len": 100
                                                 }
**/
#endif
