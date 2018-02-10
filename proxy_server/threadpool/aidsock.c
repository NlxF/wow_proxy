#include "aidsock.h"
#include "../xml/analysis_soap.h"

void write_db(sqlite3 **db, char cmds[]);
void write_pipe_or_sock(SOCKDATA *sockData, int resp, char cmd[], size_t size);
void close_db(sqlite3 *db);

__thread int _td_sock_id = 0;                          // 线程本地存储的与server soap通信的sock
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;  // rwlock use for pipe read


void print_paese_error(int err)
{
    if (err==PARAMNULL)
    {
        dbgprint("参数为空\n");
    }
    else if(err==COMMANDNOEXIST)
    {
        dbgprint("当前命令不存在\n");
    }
    else if(err==PARAMTYPEERR)
    {
        dbgprint("参数类型不正确\n");
    }
    else if(err==PARAMORDERERR)
    {
        dbgprint("参数顺序不正确\n");
    }
    else if(err==PARAMNUMERR)
    {
        dbgprint("命令参数个数不正确\n");
    }
    else if(err==DEPRECATED)
    {
        dbgprint("命令已废弃\n");
    }
    else
    {
        dbgprint("未知错误\n");
    }
}

ssize_t recv_peek(int sockfd, void *buf, size_t len)    
{    
    int ret = 0;
    int idx = 10;                                //最多尝试10次
    while (idx > 0)  
    {    
        dbgprint("read head size\n");
        ret = recv(sockfd, buf, len, MSG_PEEK);    
        if (ret == -1 && errno == EINTR)         //如果recv是由于被信号打断, 则需要继续(continue)查看
        {  
            idx--;
            continue;    
        }
        break;    
    }
    return ret;
}

/**返回值说明:  
    == count: 说明正确返回, 已经真正读取了count个字节  
    == -1   : 读取出错返回  
    <  count: 读取到了末尾  
**/
ssize_t readn(int fd, void *buf, size_t count)
{
    ssize_t nRead = 0;    
    size_t nLeft = count;   
    char *pBuf = (char *)buf;
    while (nLeft > 0)    
    {    
        if ((nRead = read(fd, pBuf, nLeft)) < 0)                            
        {    
            if (errno == EINTR)                                                                                                                                                                                                                                                                                 
                continue;          //如果读取操作是被信号打断了, 则说明还可以继续读  
            else
            {
                if (errno != EAGAIN)
                {
                    dbgprint("%s:%d:client fd=%d, Some unexpected error occurred, the error(%d) message is %s\n", __FILE__, __LINE__, fd, errno, strerror(errno));
                    return -1;
                }
                else
                {
                    //errno == EAGAIN
                    dbgprint("We have read all data of sock:%d. So go back to the main loop\n", fd);
                    return pBuf-(char*)buf;
                }
            }
        }    
        if (nRead == 0)             //读取到末尾
        {
            dbgprint("read end of socket fd=%d.\n", fd);  
            return count-nLeft;    
        }
    
        nLeft -= nRead;            //正常读取  
        pBuf += nRead;    
    }    
    return count;
}

/**返回值说明:  
    == count: 说明正确返回, 已经真正写入了count个字节  
    == -1   : 写入出错返回  
**/    
// ssize_t writen(int fd, const void *buf, size_t count)    
// {    
//     size_t nLeft = count;    
//     ssize_t nWritten = 0;    
//     char *pBuf = (char *)buf;    
//     while (nLeft > 0)    
//     {    
//         if ((nWritten = write(fd, pBuf, nLeft)) < 0)    
//         {    
//             if (errno == EINTR)    
//                 continue;          //如果写入操作是被信号打断了, 则说明还可以继续写入    
//             else    
//                 return -1;         //否则就是其他错误    
//         }    
//         else if (nWritten == 0)    //如果 ==0则说明是什么也没写入, 可以继续写    
//             continue;    
    
//         nLeft -= nWritten;         //正常写入    
//         pBuf += nWritten;    
//     }    
//     return count;    
// }

int convert_hex_to_int(unsigned char *szBuf, int headSize)
{   
    int value = 0;
    int i;
    for(i=0; i<headSize; ++i)
    {
        int bit = (int)szBuf[i];
        dbgprint("%d byte is:%x\n", i, bit);
        if(bit>255 || bit<0)
            return 0;
        
        value = value * 256 + bit;
    }

    return value;
}

bool convert_int_to_hex(unsigned int value, unsigned char*buf)
{
    if(value>0xffff || value<0)
        return false;

    memset(buf, '\x0', HEADSIZE);
    if(value <= 0xff)
        buf[1] = (unsigned char)value;
    else
    {
        buf[0] = (unsigned char)(value / 256);
        buf[1] = (unsigned char)(value % 256);
    }

    return true;
}

int read_normal_sock(int client_fd, char **szBuf)
{
    int nbytes;
    unsigned char szHeadBuf[HEADSIZE+1] = {'\0'};

    nbytes = recv_peek(client_fd, szHeadBuf, HEADSIZE);
    dbgprint("read data:%d, message size:%x%x\n", nbytes, szHeadBuf[0], szHeadBuf[1]);
    if (nbytes <= 0)                   //如果查看失败或者对端关闭, 则直接返回
        return nbytes;

    int nLeft = convert_hex_to_int(szHeadBuf, HEADSIZE);
    dbgprint("convert char string to int:%d\n", nLeft);
    if(nLeft <= 0)
        return -1;                     //如果包格式不正确

    nLeft += HEADSIZE;                 //加上头部字节
    char *p = calloc(sizeof(char), nLeft);
    int nRead = readn(client_fd, (void*)p, nLeft);
    dbgprint("read from client fd=%d, %d bytes\n", client_fd, nRead);

    *szBuf = p;
    return nRead;
}

#ifdef SOCKSSL
// int read_ssl_sock(SOCKDATA *sockData, char *szBuf, size_t size)
int read_ssl_sock(SSL *_ssl, char *szBuf, size_t size)
{
    int nbytes;
    
    // SSL *_ssl = sockData->sockConn->ssl;
    nbytes = SSL_read(_ssl, szBuf, size);
    int err = SSL_get_error(_ssl, nbytes);
    
    if (nbytes == 0)
    {
        if (err == SSL_ERROR_ZERO_RETURN)
        {
            dbgprint("SSL has been shutdown\n");
        }
        else
        {
            dbgprint("Connection has been aborted.\n");
        }
        // free_sockconn(sockData->sockConn);
        return -1;// break;
    }
    if (nbytes < 0)
    {
        fd_set fds;
        struct timeval timeout;
        switch (err)
        {
            case SSL_ERROR_NONE:
            {
                // no real error, just try again...
                dbgprint("%s:%d:%s", __FILE__, __LINE__, "SSL_ERROR_NONE \n");
                return 0;
            }
            case SSL_ERROR_ZERO_RETURN:
            {
                // peer disconnected...
                dbgprint("%s:%d:%s", __FILE__, __LINE__, "SSL_ERROR_ZERO_RETURN, SSL has been shutdown \n");
                // free_sockconn(sockData->sockConn);
                break;
            }
            case SSL_ERROR_WANT_READ:
            {
                // no data available right now, wait a few seconds in case new data arrives...
                dbgprint("%s:%d:%s", __FILE__, __LINE__, "SSL_ERROR_WANT_READ \n");
                int sock = SSL_get_rfd(_ssl);
                FD_ZERO(&fds);
                FD_SET(sock, &fds);
                
                timeout.tv_sec  = FD_TIMEOUT;
                timeout.tv_usec = 0;
                
                err = select(sock+1, &fds, NULL, NULL, &timeout);
                if (err > 0)
                {
                    return 0; // more data to read...
                }
                if (err == 0)
                {
                    dbgprint("%s:%d:%s", __FILE__, __LINE__, "wait for data coming timeout\n");
                }
                else {
                    dbgprint("%s:%d:%s", __FILE__, __LINE__, "wait for data coming occure a error\n");
                }
                break;
            }
            case SSL_ERROR_WANT_WRITE:
            {
                // socket not writable right now, wait a few seconds and try again...
                dbgprint("%s:%d:%s", __FILE__, __LINE__, "SSL_ERROR_WANT_WRITE \n");
                int sock = SSL_get_wfd(_ssl);
                FD_ZERO(&fds);
                FD_SET(sock, &fds);
                
                timeout.tv_sec  = FD_TIMEOUT;
                timeout.tv_usec = 0;
                
                err = select(sock+1, NULL, &fds, NULL, &timeout);
                if (err > 0)
                {
                    return 0; // can write more data now...
                }
                if (err == 0)
                {
                    dbgprint("%s:%d:%s", __FILE__, __LINE__, "wait to write data timeout\n");
                }
                else
                {
                    dbgprint("%s:%d:%s", __FILE__, __LINE__, "wait to write data occure a error\n");
                }
                break;
            }
            default:
            {
                dbgprint("%s:%d:SSL_read byte=%d, error=%d\n", __FILE__, __LINE__, nbytes, err);
                break;
            }
        }
        return -1;// break;
    }
    
    dbgprint("read from SSL client fd=%d ssl=%d, data=%s\n", sockData->sockConn->sock_fd, _ssl, szBuf);
    return nbytes;
}
#endif

void free_command(int resp[], char *cmds[], int is2Pipe[], int num)
{
    if (resp==NULL||cmds==NULL||is2Pipe==NULL)
        return;
    
    int i;
    for (i=0; i<num; i++)
    {
        SAFE_FREE(cmds[i]);
    }
    
    SAFE_FREE(resp);
    SAFE_FREE(is2Pipe);
    SAFE_FREE(cmds);
}

int vaild_item(cJSON *cmds, cJSON *keys, int idx, char *op[])
{
    *op = NULL;
    cJSON *obj = cJSON_GetArrayItem(keys, idx);
    if(obj == NULL)
        return PARAMTYPEERR;              //参数类型不正确
    
    char *_key = obj->valuestring;
    obj = cJSON_GetObjectItem(cmds, _key);
    if (obj == NULL)
        return PARAMTYPEERR;              //参数类型不正确
    
    if(obj->valuestring == NULL)
        return PARAMTYPEERR;              //参数类型不正确
        
    *op = obj->valuestring;
    dbgprint("key=%s,  value=%s\n", _key, *op);
    return 0;
}

int parse_object(cJSON *cmds, cJSON *keys, char**value, int *fs)
{
    if (cmds == NULL || keys == NULL)
        return PARAMNULL;                        //参数为空
    
    dbgprint("parse one json object.\n");
    char *cmdStr   = NULL;
    int needRsp    = 0;
    int paramNum   = 1;
    int deprecated = 1;
    
    *fs = 0;
    *value = NULL;
    int parmSize = cJSON_GetArraySize(keys);
    if (parmSize > 5 || parmSize <= 1)            //命令参数最大限制5个、最小2个，包括op字段
        return PARAMNUMERR;                       //命令参数个数不正确
    
    dbgprint("params num:%d.\n", parmSize);
    
    char szBuf[MAX_BUF_SIZE*2] = { 0 };
    int rtn;
    char *op;
    
    if ((rtn = vaild_item(cmds, keys, 0, &op)) < 0)
        return rtn;                                  //参数类型不正确
    
    Command *cmdObj = value_for_key(atoi(op));
    if (cmdObj != NULL)
    {
        cmdStr = cmdObj->value;
        needRsp = cmdObj->needRsp;
        deprecated = cmdObj->deprecated;
        paramNum = cmdObj->paramNum;
        *fs = cmdObj->is2Pipe;
    }
    else
        return COMMANDNOEXIST;       //当前命令不存在
    
    if(parmSize-1 != paramNum)
        return PARAMNUMERR;          //命令需要的参数个数不符
    if (deprecated==1)               //命令已废弃
        return DEPRECATED;
    
    dbgprint("Get command by key.\n");
    
    char *parms[4] = { NULL };       //参数数组, 有效参数个数为4
    switch(parmSize)
    {
        case 5:
        {
            if ((rtn = vaild_item(cmds, keys, 4, &parms[3])) < 0)
                return rtn;                                  //参数类型不正确
        }
        case 4:
        {
            if ((rtn = vaild_item(cmds, keys, 3, &parms[2])) < 0)
                return rtn;                                  //参数类型不正确
        }
        case 3:
        {
            if ((rtn = vaild_item(cmds, keys, 2, &parms[1])) < 0)
                return rtn;                                  //参数类型不正确
        }
        case 2:
        {
            if ((rtn = vaild_item(cmds, keys, 1, &parms[0])) < 0)
                return rtn;                                  //参数类型不正确
        }
            break;
        default:
            return PARAMNUMERR;
    }
    
    int bufSize = sizeof(szBuf);
    if (parmSize==2)
    {
        snprintf(szBuf, bufSize, cmdStr, parms[0]);
    }
    else if(parmSize==3)
    {
        snprintf(szBuf, bufSize, cmdStr, parms[0], parms[1]);
    }
    else if(parmSize==4)
    {
        snprintf(szBuf, bufSize, cmdStr, parms[0], parms[1], parms[2]);
    }
    else if(parmSize==5)
    {
        snprintf(szBuf, bufSize, cmdStr, parms[0], parms[1], parms[2], parms[3]);
    }
    
    *value = calloc(sizeof(char), bufSize + 1);
    strncpy(*value, szBuf, bufSize);
    
    dbgprint("Complete command:\'%s\' parse from json object.\n", *value);
    return needRsp;
}

int analysis_message(char *original_msg, size_t nbytes, char **cmds[], int *resp[], int *is2Pipe[])
{
    if (original_msg==NULL || nbytes<=0)
        return 0;
    
    nbytes -= HEADSIZE;                      // 减去头部字节
    original_msg += HEADSIZE;
    uint8_t *podata = calloc(sizeof(uint8_t), nbytes);
    if(reverse_crc_data(original_msg, nbytes, (void*)podata)!=0)
    {
        dbgprint("The data from client is invalid: crc error\n");
        SAFE_FREE(podata);
        return 0;
    }
    //parse JSON
    dbgprint("recv json data:%s\n", podata);
    cJSON *root = cJSON_Parse((char*)podata);
    if (root == NULL)
    {
        const char *err_msg = cJSON_GetErrorPtr();
        dbgprint("Parse JSON error:%s\n", err_msg);
        SAFE_FREE(podata);
        return 0;
    }
    SAFE_FREE(podata);

    cJSON *cmdsArray = cJSON_GetObjectItem(root, "values");   //命令数组
    cJSON *keysArray = cJSON_GetObjectItem(root, "keys");     //键
    if(cmdsArray==NULL || keysArray==NULL)
    {
        dbgprint("values or keys is NULL\n");
        cJSON_Delete(root);
        return 0;
    }
    
    int cmdSize = cJSON_GetArraySize(cmdsArray);
    int keySize = cJSON_GetArraySize(keysArray);
    if (keySize<=0 || cmdSize<=0)
    {
        dbgprint("values or keys size is 0\n");
        cJSON_Delete(root);
        return 0;
    }
    
    keySize = keySize>cmdSize?cmdSize:keySize;                //取小
    dbgprint("key‘s array num=%d.\n", keySize);
    
    int *rs = calloc(sizeof(int), keySize);
    char **values = calloc(sizeof(char*), keySize);
    int *fs = calloc(sizeof(int), keySize);                   //写入对象
    
    dbgprint("start parse json object.\n");
    int i;
    for (i=0; i<keySize; i++)
    {
        cJSON *one_cmd = cJSON_GetArrayItem(cmdsArray, i);
        cJSON *one_key = cJSON_GetArrayItem(keysArray, i);
        
        rs[i] = parse_object(one_cmd, one_key, &values[i], &fs[i]);
        if (rs[i] < 0)
        {
            print_paese_error(rs[i]);                         //解析是否出错
            cJSON_Delete(root);                               //有一个错误整串命令都放弃
            SAFE_FREE(rs);
            SAFE_FREE(fs);
            SAFE_FREE(values);
            return 0;
        }
    }
    *resp = rs;
    *cmds = values;
    *is2Pipe = fs;
    
    cJSON_Delete(root);
    
    return keySize;
}

// 在线程池中随机的一条线程中运行
void *read_sock_func(void *p)
{
    if(p == NULL)
        return NULL;

    char szBuf[MAX_BUF_SIZE] = {0};
    char *pszBuf = NULL;
    SOCKDATA *sockData = (SOCKDATA*)p;
    
    sqlite3 *db = NULL;
    int sock = sockData->sockConn->sock_fd;
#ifdef SOCKSSL
    SSL *ssl = sockData->sockConn->ssl;
#endif
    while(1)
    {
        int nbytes;
#ifndef SOCKSSL
        nbytes = read_normal_sock(sock, &pszBuf);
#else
        nbytes = read_ssl_sock(ssl, szBuf, sizeof(szBuf));
#endif
        if (nbytes < 0)
        {
            dbgprint("read from sock:%d failed\n", sock);
            SAFE_FREE(pszBuf);
            break;
        }
        
        int *resp = NULL;        //是否有效命令
        char **cmds = NULL;      //命令字符串
        int *is2Pipe = NULL;     //操作类型,写入管道还是数据库
        
        dbgprint("start analysis message\n");
        int num = analysis_message(pszBuf, nbytes, &cmds, &resp, &is2Pipe);     //返回命令数组
        SAFE_FREE(pszBuf);
        if (num <= 0)
        {
            dbgprint("analysis message error: no valid command.\n");
            break;
        }
        dbgprint("analysis message finish, start to write.\n");
        
        int idx;
        for(idx=0; idx<num; idx++)
        {
            if(resp[idx] >=0 && cmds[idx] != NULL)
            {
                if(is2Pipe[idx] == 1)
                {
                    dbgprint("write pipe/sock message: %s\n", cmds[idx]);
                    write_pipe_or_sock(sockData, resp[idx], cmds[idx], strnlen(cmds[idx], nbytes)); // need return
                    dbgprint("write pipe/sock message finished!!!\n");
                }
                else
                {
                    dbgprint("write database message: %s\n", cmds[idx]);
                    write_db(&db, cmds[idx]);   // need return
                }
            }
        }
        
        free_command(resp, cmds, is2Pipe, num);
    }
    free_sockData(sockData);
    
    if(db != NULL)
    {
        close_db(db);
    }

    dbgprint("Read job finish with sock:%d\n", sock);
	return NULL;
}


/**
 写入管道或socket
 @param sockData SOCKDATA结构体
 @param resp 是否需要返回
 @param cmd 写入的命令
 @param size 命令长度
 */
void write_pipe_or_sock(SOCKDATA *sockData, int resp, char cmd[], size_t size)
{
    int nbytes;
    int fd_read;                         //读取数据的fd,读管道或者sock
    int fd_write;                        //写入数据的fd,读管道或者sock
    char szBuf[MAX_BUF_SIZE*4] = {0};
    
#ifdef _SOAP
    int sock_fd;
    if((sock_fd=make_soap_socket()) <= 0)   
    {
        // dbgprint("%s:%d:%s\n", __FILE__, __LINE__, "make soap socket failed!!!");
        write_fd_error_message(sockData, 1);
        return;
    }
    fd_write = sock_fd;
    fd_read  = sock_fd;
    size = make_soap_request(cmd, size, szBuf);
#else
    // 命名管道的最大BUF是64kb,但是最大原子写是4KB.
    // 加锁是为了保证如果需要读，当前读出的就是写入的返回
    pthread_rwlock_wrlock(&rwlock);
    fd_write = sockData->write_fd;
    fd_read  = sockData->read_fd;
    memcpy(szBuf, cmd, size);
#endif
    if(fd_write > 0)
    {
        //等待发送完所有的data, 若为sock且在对端已关闭，则创建新的
        int ret = writen_fd(&fd_write, szBuf, size);        
        if(ret <= 0)
        {
            //write soap or pipe faile
#ifdef _SOAP
            write_fd_error_message(sockData, 1);
#else
            write_fd_error_message(sockData, 2);
            pthread_rwlock_unlock(&rwlock);                 // if broken pipe
#endif
            return;
        }

        fd_read = fd_write;                                 // 尝试更新
        nbytes = readn_fd(fd_read, szBuf, sizeof(szBuf));   // 尝试接收返回值
        dbgprint("read data from pipe/soap sock=%d with size:%d\n", fd_read, nbytes);
        dbgprint("need response?%s\n", resp==0?"NO":"YES");
        if(resp == 1 && nbytes > 0)
        {
#ifdef _SOAP
            char szrst[MAX_BUF_SIZE*4] = {0};
            size_t rst_len = sizeof(szrst);
            sockData->isOpOk = analysis_soap_response(szBuf, nbytes, szrst, &rst_len);
            memcpy(szBuf, szrst, rst_len);
            szBuf[rst_len] = '\0';
            nbytes = rst_len;
#endif
            sockData->msg = szBuf;
            sockData->size = nbytes;
            write_sock_func(sockData);
        }
        else if(nbytes == 0)
        {
            // sock is closed by peer
            dbgprint("sock:%d is closed by peer\n", fd_read);
        }
    }
#ifndef _SOAP
    pthread_rwlock_unlock(&rwlock);
#endif
}


void write_db(sqlite3 **_db, char cmds[])
{
    if(cmds==NULL || strlen(cmds)<=0)
        return;
    
    int rc;
    sqlite3 *db = *_db;
    if (db == NULL)
    {
        char buf[MAX_BUF_SIZE]     = {0};
        char db_path[MAX_BUF_SIZE] = {0};
        
        getcwd(buf, sizeof(buf));
        snprintf(db_path, sizeof(db_path), "%s/%s", buf, "commands.db");
        
        rc = sqlite3_open(db_path, &db);
        if( rc )
        {
            dbgprint("%s:%d:%s: %s: %s\n", __FILE__, __LINE__, "Can't open database", sqlite3_errmsg(db), db_path);
            close_db(db);
            return;
        }
        *_db = db;
        dbgprint("Opened database:%p successfully\n", db);
    }
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, cmds, -1, &stmt, NULL);    // Prepare a query

    rc = sqlite3_step(stmt);                          // Execute the statement                           
    if (rc != SQLITE_DONE) 
    {
        dbgprint("%s:%d:%s: %s\n", __FILE__, __LINE__, "ERROR ", sqlite3_errmsg(db));
        close_db(db);
        return;
    }

    sqlite3_finalize(stmt);                           // Free the ressources by finalizing the statement

    // char *zErrMsg = NULL;
    // rc = sqlite3_exec(db, cmds, 0, 0, &zErrMsg);
    // if( rc != SQLITE_OK )
    // {
    //     dbgprint("%s:%d:%s: %s\n", __FILE__, __LINE__, "SQL execute error", zErrMsg);
    //     sqlite3_free(zErrMsg);
    //     // close_db();
    //     return;
    // }
}

void close_db(sqlite3 *db)
{
   /*close db*/
   if (db != NULL)
   {
       sqlite3_close(db);
       dbgprint("close db:%p\n", db);
       db = NULL;
   }
}

/* 
函数有两个进入点
一是从epoll循环的handle_write里进入,
二是从epoll循环的handle_read->read_sock_func->write_pipe_or_sock进入
两种情况下会是在线程池中随机的一条线程中运行
*/
void *write_sock_func(void *p)
{
    if(p == NULL)
	    return NULL;

    SOCKDATA *sockData = (SOCKDATA*)p;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "isopok", sockData->isOpOk);
    cJSON_AddStringToObject(root, "message", sockData->msg);
    cJSON_AddNumberToObject(root, "len", sockData->size);
    char *json_str = cJSON_Print(root);

    uint8_t buf[MAX_BUF_SIZE*4] = {0};
	size_t size = strlen(json_str);
    if(size > sizeof(buf))
    {
        cJSON_Delete(root);
        return NULL;
    }
    
    int len;
	if((len=crc_data(json_str, size, buf+HEADSIZE))==-1)
	{
        cJSON_Delete(root);
		return NULL;
	}
    
    convert_int_to_hex(len, buf);
    len += HEADSIZE;
	int nbytes;
#ifndef SOCKSSL
    int client_fd = sockData->sockConn->sock_fd;
    nbytes = writen_fd(&client_fd, buf, len);
#else
    SSL *_ssl = sockData->sockConn->ssl;
    nbytes = SSL_write(_ssl, buf, len);
#endif

    SAFE_FREE(json_str);
    cJSON_Delete(root);
    
    dbgprint("write job finish!!!\n");
	return NULL;
}

void *free_sockConn_func(void *p)
{
    if(p != NULL)
    {
        SOCKCONN* sockConn = (SOCKCONN*)p;
        free_sockconn(sockConn);
    }

    return NULL;
}