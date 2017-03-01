#include "aidsock.h"
#include "sys/select.h"
#include "unistd.h"

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;  //rwlock

int read_fd(int read_fd, char *szData, size_t nData)
{
    if(read_fd < 0)
    {
        return -1;
    }
    
	bzero(szData, nData);

	fd_set rfds;
	struct timeval tv;

	FD_ZERO(&rfds);
	FD_SET(read_fd, &rfds);
	
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	
	int totalByte = 0;
	int retval = select(read_fd+1, &rfds, NULL, NULL, &tv);
	if(retval > 0)
	{
		usleep(100*1000);            //微秒，等待0.1秒
		if(FD_ISSET(read_fd, &rfds))
		{
			char szBuf[256] = {0};
			ssize_t nReadByte;
			do{
				nReadByte = read(read_fd, szBuf, 256);
				totalByte += nReadByte;
				if(totalByte < nData)
					strcat(szData, szBuf);
				else{
					totalByte -= nReadByte;
					break;
				}
			}while(nReadByte == 256);
		}
	}
    else
    {
		sprintf(szData, "%s", "The operation was timed out");
    }

	return totalByte;
}


char *analysis_message(char *original_msg, size_t nbytes, int *resp)
{
    if (original_msg==NULL || nbytes<=0 || nbytes>MAX_SIZE)
    {
        return NULL;
    }
    
//    uint8_t *podata = (uint8_t*)malloc(nbytes);
    uint8_t podata[MAX_SIZE] = {0};
    if(reverse_crc_data(original_msg, nbytes, (void*)podata)!=0)
    {
        dbgprint("The data from client is invalid: crc error");
//        if(podata) free(podata);
        //close (client_fd);
        return NULL;
    }
    //parse JSON
    dbgprint("recv json data:%s\n", podata);
    cJSON *root = cJSON_Parse((char*)podata);
    if (root == NULL)
    {
        const char *err_msg = cJSON_GetErrorPtr();
        dbgprint("Parse JSON error:%s\n", err_msg);
//        free(podata);
        return NULL;
    }
    
    int needResponse = cJSON_GetObjectItem(root, "needResponse")->valueint;
    char *message = cJSON_GetObjectItem(root, "message")->valuestring;     //后面要把message换成索引，从数据库里读
    int msgLen = cJSON_GetObjectItem(root, "len")->valueint;
    
    bzero(original_msg, nbytes);
    strncpy(original_msg, message, msgLen);
    original_msg[msgLen] = '\n';
    original_msg[msgLen+1] = '\0';
    
    *resp = 0;
    
    cJSON_Delete(root);
    
    return original_msg;
}


void *read_sock(void *p)
{
    if(p == NULL)
    {
        return NULL;
    }

    char szBuf[MAX_SIZE*4] = {0};
    SOCKDATA *sockData = (SOCKDATA*)p;
    
    while(1)
    {
        int nbytes;
#ifndef SOCKSSL
        int client_fd = sockData->sockConn->sock_fd;
        nbytes = read(client_fd, szBuf, sizeof(szBuf));
        if(nbytes == -1)
        {
            dbgprint("%s:%d:client fd=%d, read -1 and the errno is %d\n", __FILE__, __LINE__, client_fd, errno);
            if (errno != EAGAIN)
            {
                //If errno == EAGAIN, that means we have read all data. So go back to the main loop
                close (client_fd);
            }
            break;
        }
        else if (nbytes == 0)
        {
            //End of file. The remote has closed the connection.
            dbgprint("%s:%d:End of socket fd=%d. The remote has closed the connection.\n", __FILE__, __LINE__, client_fd);
            close (client_fd);
            break;
        }
        dbgprint("read from client fd=%d\n", client_fd);
#else
        SSL *_ssl = sockData->sockConn->ssl;
        nbytes = SSL_read(_ssl, szBuf, sizeof(szBuf));
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
            break;
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
                    continue;
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
                    
                    timeout.tv_sec  = 5;
                    timeout.tv_usec = 0;
                    
                    err = select(sock+1, &fds, NULL, NULL, &timeout);
                    if (err > 0)
                    {
                        continue; // more data to read...
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
                    
                    timeout.tv_sec  = 5;
                    timeout.tv_usec = 0;
                    
                    err = select(sock+1, NULL, &fds, NULL, &timeout);
                    if (err > 0)
                    {
                        continue; // can write more data now...
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
            break;
        }
        
        dbgprint("read from SSL client fd=%d ssl=%d, data=%s\n", sockData->sockConn->sock_fd, _ssl, szBuf);
#endif
        int resp = 0;
//        char *format_message = analysis_message(szBuf, nbytes, &resp);
        char *format_message = szBuf;
        if (format_message==NULL)
        {
            break;
        }

        //write message to pipe note：命名管道的最大BUF是64kb,但是最大原子写是4KB
        //加锁是为了保证如果需要读，当前读出的就是写入的返回
        pthread_rwlock_wrlock(&rwlock);
        write(sockData->write_fd, szBuf, strnlen(format_message, nbytes));
		
		dbgprint("need response?%s\n", resp==0?"NO":"YES");
        if(resp)
        {
			dbgprint("Request need response\n");
			nbytes = read_fd(sockData->read_fd, szBuf, sizeof(szBuf));
			if(nbytes > 0)
            {
				dbgprint("read data:%s and ready to send\n", szBuf);
				sockData->msg = szBuf;
            	sockData->size = nbytes;
            	write_sock(sockData);
			}	
        }
        pthread_rwlock_unlock(&rwlock);
    }
    
    free_sockData(sockData);
    dbgprint("Read job Finish\n");
    
	return NULL;
}

void *write_sock(void *p)
{
    if(p == NULL)
    {
	    return NULL;
    }

    SOCKDATA *sockData = (SOCKDATA*)p;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "needResponse", 0);
    cJSON_AddStringToObject(root, "message", sockData->msg);
    cJSON_AddNumberToObject(root, "len", sockData->size);
    char *json_str = cJSON_Print(root);

	int len;
	size_t size = strlen(json_str);
    if(size > MAX_SIZE)
    {
        return NULL;
    }
    uint8_t buf[MAX_SIZE] = {0};
	if((len=crc_data(json_str, size, buf))==-1)
	{
        cJSON_Delete(root);
		return NULL;
	}

	int nbytes;
	//nbytes =send(sockData->sockConn->sock_fd, buf, len, 0);
#ifndef SOCKSSL
    int client_fd = sockData->sockConn->sock_fd;
    nbytes = write(client_fd, buf, len);
#else
    SSL *_ssl = sockData->sockConn->ssl;
    nbytes = SSL_write(_ssl, buf, len);
#endif
    cJSON_Delete(root);

	return NULL;
}
