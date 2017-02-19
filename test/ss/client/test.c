#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "crc8.h"
#include "cJSON.h"


int port = 8083;
char HOST[] = "127.0.0.1";

void *do_work(void *p)
{
	int sock_fd;
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in srv;
	srv.sin_family = AF_INET;
	srv.sin_port = htons(port);
	srv.sin_addr.s_addr = inet_addr(HOST);
	connect(sock_fd, (struct sockaddr*)&srv, sizeof(srv));

	char szBuf[50];
	snprintf(szBuf, 50, "%s %d", p, pthread_self());
	printf(szBuf);

    //use json
    cJSON *root_json = cJSON_CreateObject();
    cJSON_AddItemToObject(root_json, "needResponse", cJSON_CreateTrue());
    cJSON_AddItemToObject(root_json, "message", cJSON_CreateString(szBuf));
    cJSON_AddItemToObject(root_json, "len", cJSON_CreateNumber(strlen(szBuf)));

    char *json_str = cJSON_Print(root_json);

    size_t size = strlen(json_str);
	uint8_t *pbuf = (uint8_t*)malloc(sizeof(uint8_t)*(size+1));
	crc_data(json_str, size, pbuf);

	time_t t1, t2;
	time(&t1);
	send(sock_fd, pbuf, size+1, 0);

	//recv(sock_fd, pbuf, size+1, 0);
	time(&t2);
	printf("all spend %dsec\n", t2-t1);

	free(pbuf);
}

int main()
{
	int i;
	int num = 2;

    printf("create therads pool...");
	pthread_t *threads;
	threads = (pthread_t*)malloc(sizeof(pthread_t)*num);

	//char *p = (char*)malloc(sizeof(char)*50);
	//bzero(p, 50);
	//snprintf(p, 50, "hello world");
	//do_work(p);
    printf("add jobs to thread pool...");
	for(i=0; i<num; ++i)
	{
		char param[50] = {0};
		snprintf(param, 50, "%d thread:", i+1);
		pthread_create(&threads[i], NULL, do_work, param);
	}

	for(i=0; i<num; ++i)
	  pthread_join(threads[i], NULL);

    printf("test结束...");
	free(threads);
}
