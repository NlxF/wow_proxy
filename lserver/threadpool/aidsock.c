#include "aidsock.h"
#include "sys/select.h"
#include "unistd.h"

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;  //rwlock

int read_fd(int read_fd, char *szData, size_t nData)
{
	bzero(szData, nData);
	if(read_fd < 0)
		return -1;

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
		usleep(100*1000);
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
		sprintf(szData, "%s", "The operation was timed out");

	return totalByte;
}

void *read_sock(void *p)
{
    if(p==NULL)
        return NULL;

    char szBuf[MAX_SIZE*5] = {0};
    SOCKDATA *pdata = (SOCKDATA*)p;
    int idx = 0;
    while(1)
    {
        int nbytes;
        //nbytes=recv(pdata->sock_fd, szBuf, MAX_SIZE, 0);
        nbytes = read(pdata->sock_fd, szBuf, MAX_SIZE*5);
        if(nbytes == -1)
        {
            printf("index:%d, client: %d, read -1 and the errno is %d\n", idx++, pdata->sock_fd, errno);
            if (errno != EAGAIN)
            {
                //If errno == EAGAIN, that means we have read all data. So go back to the main loop
                close (pdata->sock_fd);
            }
            break;
        }
        else if (nbytes == 0)
        {
            //End of file. The remote has closed the connection.
            printf("index: %d, End of socket:%d. The remote has closed the connection.\n", idx++, pdata->sock_fd);
            close (pdata->sock_fd);
            break;
        }
        printf("index:%d, client: %d\n", idx++, pdata->sock_fd);
        uint8_t *podata = (uint8_t*)malloc(nbytes-1);
        if(reverse_crc_data(szBuf, nbytes, (void*)podata)!=0)
        {
            psyslog(LOG_ERR, "The data from client is invalid: crc error");
            free(podata);
            close (pdata->sock_fd);
            break;
        }
        //parse JSON
		printf("recv json data:%s\n", podata);
        cJSON *root = cJSON_Parse((char*)podata);
        if (root == NULL)
        {
            psyslog(LOG_ERR, cJSON_GetErrorPtr());
            free(podata);
            break;
        }

        int needResponse = cJSON_GetObjectItem(root, "needResponse")->valueint;
        char *message = cJSON_GetObjectItem(root, "message")->valuestring;
        int msgLen = cJSON_GetObjectItem(root, "len")->valueint;

        bzero(szBuf, MAX_SIZE*5);
        strncpy(szBuf, message, msgLen);
        szBuf[msgLen] = '\n';

        pthread_rwlock_wrlock(&rwlock);
        write(pdata->write_fd, szBuf, msgLen+1);
		
		printf("need response?%s\n", needResponse==1?"YES":"NO");
        if(needResponse)
        {
			printf("Request need response\n");
			nbytes = read_fd(pdata->read_fd, szBuf, MAX_SIZE*5);
			if(nbytes > 0)
            {
				printf("read data:%s and ready to send\n", szBuf);
				pdata->msg = szBuf;
            	pdata->size = nbytes;
            	write_sock(pdata);
			}	
        }
        pthread_rwlock_unlock(&rwlock);

        free(podata);
        cJSON_Delete(root);
    }

    free(pdata);

	return NULL;
}

void *write_sock(void *p)
{
    if(p == NULL)
	    return NULL;

    SOCKDATA *pdata = (SOCKDATA*)p;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "needResponse", 0);
    cJSON_AddStringToObject(root, "message", pdata->msg);
    cJSON_AddNumberToObject(root, "len", pdata->size);
    char *json_str = cJSON_Print(root);

	int len;
	size_t size = strlen(json_str);
	uint8_t *buf = (uint8_t*)malloc(size+5);
	if((len=crc_data(json_str, size, buf))==-1)
	{
        free(buf);
        cJSON_Delete(root);
		return NULL;
	}

	int nbytes;
	//nbytes =send(pdata->sock_fd, buf, len, 0);
    nbytes = write(pdata->sock_fd, buf, len);
	free(buf);
    cJSON_Delete(root);

	return NULL;
}
