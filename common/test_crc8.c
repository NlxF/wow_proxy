#include <stdio.h>
#include <stdlib.h>
#include "crc8.h"

char* itoa(int val, int base)
{
		
	static char buf[32] = {0};
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = "0123456789abcdef"[val % base];					
	return &buf[i+1];
}

int main()
{
	uint8_t       *pdata;
	char szBuf[256]  = {0};
	char szBuf2[256]  = {0};
	char szMsg[] = {"hello world!qeqweqeqweqeqw"};
	size_t len = strlen(szMsg);
	pdata = (uint8_t*)malloc(len);
	memcpy(pdata, szMsg, len);

	//uint8_t crcVal = Crc8((void*)pdata, len);
	//char *szTmp;
	//szTmp = itoa(crcVal, 2);
	len = crc_data((void*)pdata, len, szBuf);
	printf("thr crc value of string: %s is %s\n", szMsg, szBuf);
	printf("%x\n", szBuf[len-1]);

	reverse_crc_data(szBuf, len, szBuf2);
    printf("thr reverse crc value of string: %s is %s\n", szBuf, szBuf2);
	free(pdata);
}
