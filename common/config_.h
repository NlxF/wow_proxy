#ifndef  _CONFIG_H
#define  _CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <syslog.h>

#ifndef SOCKSSL
#define SOCKSSL2
#endif

#ifndef _SOAP
#define _SOAP
#endif

#ifdef SOCKSSL
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#ifdef _SOAP
// #define SOAPSERVERIP "127.0.0.1"
// #define SOAPSERVERPORT 7878
#define SOAPSERVERIP "127.0.0.1"
#define SOAPSERVERPORT 18080
#else
#define LSERVER2WSERVER ".lserver2wserver"
#define WSERVER2LSERVER ".wserver2lserver"
#endif

#define MAX_BUF_SIZE 1024
#define LISTEN_MAX 500

#define ERRNUMTOEXIT  5

#define psyslog(level, format, ...) \
syslog(level, format, ##__VA_ARGS__)

#define PRINT_BASE(format, ...)    printf(format, ##__VA_ARGS__)
#define PRINT_0( format, ... ) \
do{ \
time_t timep; \
time( &timep ); \
struct tm *pTM = gmtime( &timep ); \
PRINT_BASE( "[%4d-%02d-%02d %02d:%02d:%02d]" format, \
pTM->tm_year+1900, pTM->tm_mon+1, pTM->tm_mday, pTM->tm_hour+8, pTM->tm_min, pTM->tm_sec, ##__VA_ARGS__ ) ; \
}while (0)

#define dbgprint(format, ...)  PRINT_0(format, ##__VA_ARGS__)

#ifdef SOCKSSL
typedef  int (*VerifyCallback)(int, X509_STORE_CTX *);
#endif

typedef struct
{
    int ep_fd;
    int events;
	int sock_fd;
#ifdef SOCKSSL
    SSL *ssl;
    bool sslConnected;
#endif
}SOCKCONN;

typedef struct
{
#ifdef _SOAP
    void* sock_table;         //sock hash table
#else
    int  read_fd;             //pipe read fd
    int  write_fd;            //pipe write fd
#endif
    SOCKCONN*  sockConn;      //connect info
    int  size;                //message len
    char *msg;                //send message
}SOCKDATA;


typedef struct
{
    char value[256];
    int paramNum;
    bool needRsp;
    bool deprecated;
    bool is2Pipe;             //this command is for db or worldserver
} Command;




/** JSON消息结构
client ------------------------>web server----------------------------------->wow server
                {                                {
                    "type":1                        “needResponse” : 1,
                    "command": "xxxx"               "message": "xxxxx";
                }                                   "len": 100
                                                 }
**/
#endif
