#include "aidsock.h"
#include "../xml/analysis_soap.h"


void write2db(char cmds[]);
void write2pipe(SOCKDATA *sockData, int resp, char cmd[], size_t size);

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;  //rwlock


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

/*
#ifdef _SOAP
int socket_for_key(table sock_table, int key)
{
    if(sock_table)
    {
        dbgprint("search socks table for key:%d ", key);
        char* s = make_key_by_int(key);
        elem em = (elem)table_search(sock_table, s);
        if (em)
        {
            int *ptr = em->info;
            if (ptr)
            {
                dbgprint("value:%d.\n\n", *ptr);
                return *ptr;
            }
        }
    }
    
    return 0;
}

int update_value_for_key(table sock_table, int key, int sock)
{
    if(sock_table)
    {
        dbgprint("update socks table for key:%d, with value:%d\n", key, sock);
        char* s = make_key_by_int(key);
        elem em = (elem)table_search(sock_table, s);
        if (em)
        {
            int *ptr = em->info;
            if (ptr)
                return (*ptr = sock);
        }
    }
    return -1;
}

void* init_soap_socket_table(pthread_t* pthreads, int thread_num)
{
    if (pthreads == NULL || thread_num <=0)
        return 0;
    
    int sock_num = thread_num;
    
    table socket_table = table_new( sock_num );
    if(!socket_table)
    {
        dbgprint("%s:%d:%s\n", __FILE__, __LINE__, "create socket table failed\n");
        return 0;
    }
    
    int idx=0;
    for (; idx<sock_num; idx++)
    {
        int sock = make_soap_socket();
        if (sock > 0)
        {
            dbgprint("%s:%d:%s\n", __FILE__, __LINE__, "create soap socket success");
            
            // create a elem
            elem e = malloc(sizeof(struct elem));
            e->word = make_key_by_int(pthreads[idx]);
            int *ptr = malloc(sizeof(int));
            *ptr = sock;
            e->info = (void*)ptr;
            
            table_insert(socket_table, e);
        }
        else
        {
            dbgprint("%s:%d:%s\n", __FILE__, __LINE__, "create soap socket failed");
        }
    }
    return (void*)socket_table;
}

#endif
*/

int read_normal_sock(SOCKDATA *sockData, char *szBuf, size_t size)
{
    int nbytes;
    
    int client_fd = sockData->sockConn->sock_fd;
    nbytes = read(client_fd, szBuf, size);
    if(nbytes == -1)
    {
        dbgprint("%s:%d:client fd=%d, read -1 and the error message is %s\n", __FILE__, __LINE__, client_fd, strerror(errno));
        if (errno != EAGAIN)
        {
            //If errno == EAGAIN, that means we have read all data. So go back to the main loop
            close (client_fd);
        }
        return -1;// break;
    }
    else if (nbytes == 0)
    {
        //End of file. The remote has closed the connection.
        dbgprint("%s:%d:End of socket fd=%d. The remote has closed the connection.\n", __FILE__, __LINE__, client_fd);
        close (client_fd);
        return -1;// break;
    }
    dbgprint("read from client fd=%d\n", client_fd);
    
    return nbytes;
}

#ifdef SOCKSSL
int read_ssl_sock(SOCKDATA *sockData, char *szBuf, size_t size)
{
    int nbytes;
    
    SSL *_ssl = sockData->sockConn->ssl;
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
        free_sockconn(sockData->sockConn);
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
                free_sockconn(sockData->sockConn);
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
        if (cmds[i] != NULL)
        {
            free(cmds[i]);
        }
    }
    
    free(resp);
    free(is2Pipe);
    free(cmds);
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
    char *cmdStr;
    int needRsp = 0;
    int paramNum = 1;
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
        return COMMANDNOEXIST;   //当前命令不存在
    
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
    if (original_msg==NULL||nbytes<=0)
        return 0;
    
    uint8_t podata[MAX_BUF_SIZE] = {0};
    if(reverse_crc_data(original_msg, nbytes, (void*)podata)!=0)
    {
        dbgprint("The data from client is invalid: crc error");
        return 0;
    }
    //parse JSON
    dbgprint("recv json data:%s\n", podata);
    cJSON *root = cJSON_Parse((char*)podata);
    if (root == NULL)
    {
        const char *err_msg = cJSON_GetErrorPtr();
        dbgprint("Parse JSON error:%s\n", err_msg);
        return 0;
    }
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
        }
    }
    *resp = rs;
    *cmds = values;
    *is2Pipe = fs;
    
    cJSON_Delete(root);
    
    return keySize;
}


void *read_sock_func(void *p)
{
    if(p == NULL)
        return NULL;

    char szBuf[MAX_BUF_SIZE*4] = {0};
    SOCKDATA *sockData = (SOCKDATA*)p;
    
    while(1)
    {
        int nbytes;
#ifndef SOCKSSL
        nbytes = read_normal_sock(sockData, szBuf, sizeof(szBuf));
#else
        nbytes = read_ssl_sock(sockData, szBuf, sizeof(szBuf));
#endif
        if (nbytes < 0)
            break;
        
        int *resp = NULL;        //是否有效命令
        char **cmds = NULL;      //命令字符串
        int *is2Pipe = NULL;     //操作类型,写入管道还是数据库
        
        dbgprint("start analysis message\n");
        int num = analysis_message(szBuf, nbytes, &cmds, &resp, &is2Pipe);     //返回命令数组
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
                    write2pipe(sockData, resp[idx], cmds[idx], strnlen(cmds[idx], nbytes));
                }
                else
                {
                    dbgprint("write database message: %s\n", cmds[idx]);
                    write2db(cmds[idx]);
                }
            }
        }
        
        free_command(resp, cmds, is2Pipe, num);
    }
    
    free_sockData(sockData);
    
    // close_db();
    
    dbgprint("Read job Finish\n");
    
	return NULL;
}


/**
 写入管道或socket
 @param sockData SOCKDATA结构体
 @param resp 是否需要返回
 @param cmd 写入的命令
 @param size 命令长度
 */
void write2pipe(SOCKDATA *sockData, int resp, char cmd[], size_t size)
{
    int nbytes;
    int fd_read;                         //读取数据的fd,读管道或者sock
    int fd_write;                        //写入数据的fd,读管道或者sock
    char szBuf[MAX_BUF_SIZE*4] = {0};
    
#ifdef _SOAP
    /*
    int sock_fd = socket_for_key((table)sockData->sock_table, (int)pthread_self()); //get socket
    fd_write = sock_fd;
    fd_read  = sock_fd;
    */
    int num=0;
    int sock_fd;
    while((sock_fd=make_soap_socket()) <= 0)
    {
        if(num++ >= 3)
            break;
    }
    fd_write = sock_fd;
    fd_read  = sock_fd;
    size = make_soap_request(cmd, size, szBuf);
#else
    /*
     命名管道的最大BUF是64kb,但是最大原子写是4KB.
     加锁是为了保证如果需要读，当前读出的就是写入的返回
    */
    pthread_rwlock_wrlock(&rwlock);
    fd_write = sockData->write_fd;
    fd_read  = sockData->read_fd;
    memcpy(szBuf, cmd, size);
#endif
    if (fd_write > 0)
    {
        int ret = writen_fd(fd_write, szBuf, size);         //block sock

        nbytes = readn_fd(fd_read, szBuf, sizeof(szBuf));
        dbgprint("read data from pipe/soap sock=%d:\n%s\n", fd_read, szBuf);
        dbgprint("need response?%s\n", resp==0?"NO":"YES");
        if(resp == 1 && nbytes > 0)
        {
#ifdef _SOAP
            char szrst[MAX_BUF_SIZE*4] = {0};
            size_t rst_len = sizeof(szrst);
            analysis_soap_response(szBuf, nbytes, szrst, &rst_len);
            memcpy(szBuf, szrst, rst_len);
            szBuf[rst_len] = '\0';
            nbytes = rst_len;
#endif
            sockData->msg = szBuf;
            sockData->size = nbytes;
            write_sock_func(sockData);
        }
        if (fd_write==fd_read)
        {
            close(fd_read);     //close soap sock
        }
    }
#ifndef _SOAP
    pthread_rwlock_unlock(&rwlock);
#endif
}

//static sqlite3 *db;
void write2db(char cmds[])
{
    if(cmds==NULL || strlen(cmds)<=0)
        return;
    
    int rc;
    sqlite3 *db;
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
            return;
        }
        dbgprint("Opened database successfully\n");
    }
    
    char *zErrMsg = NULL;
    rc = sqlite3_exec(db, cmds, 0, 0, &zErrMsg);
    if( rc != SQLITE_OK )
    {
        dbgprint("%s:%d:%s: %s\n", __FILE__, __LINE__, "SQL execute error", zErrMsg);
        sqlite3_free(zErrMsg);
        // close_db();
        return;
    }
    sqlite3_close(db);
    // close_db();
}

//void close_db()
//{
//    /*close db*/
//    if (db != NULL)
//    {
//        sqlite3_close(db);
//        db == NULL;
//    }
//}

void *write_sock_func(void *p)
{
    if(p == NULL)
	    return NULL;

    SOCKDATA *sockData = (SOCKDATA*)p;

    cJSON *root = cJSON_CreateObject();
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
	if((len=crc_data(json_str, size, buf))==-1)
	{
        cJSON_Delete(root);
		return NULL;
	}

	int nbytes;
	//nbytes =send(sockData->sockConn->sock_fd, buf, len, 0);
#ifndef SOCKSSL
    int client_fd = sockData->sockConn->sock_fd;
    nbytes = writen_fd(client_fd, buf, len);
#else
    SSL *_ssl = sockData->sockConn->ssl;
    nbytes = SSL_write(_ssl, buf, len);
#endif
    cJSON_Delete(root);
    
	return NULL;
}
