#include "crc8.h"

/**
 * Return CRC-8 of the data, using x^8 + x^2 + x + 1 polynomial.  A table-based
 * algorithm would be faster, but for only a few bytes it isn't worth the code
 * size. */
uint8_t crc8(const void *vptr, int len)
{
    const uint8_t *data = vptr;
    unsigned crc = 0;
    int i, j;
    for (j = len; j; j--, data++) {
        crc ^= (*data << 8);
        for(i = 8; i; i--) {
            if (crc & 0x8000)
                crc ^= (0x1070 << 3);
            crc <<= 1;
        }
    }
    return (uint8_t)(crc >> 8);
}


int is_data_integrity(const void *vptr, int len)
{
	return crc8(vptr, len);
}

int crc_data(const void* vptr, int len, void*buf)
{
	if(vptr == NULL)
	  return -1;

	char szBuf[2] = {0};
	uint8_t crcVal = crc8(vptr, len);
    strncpy(szBuf,(char*)&crcVal, 1);
	memcpy(buf, vptr, len);
	memcpy(buf+len, szBuf, 1);
	return len+1;
}

int reverse_crc_data(const void *vptr, int len, void *buf)
{
	int ret = -1;
	if(vptr && buf && (is_data_integrity(vptr, len) == 0))
	{
		memcpy(buf, vptr, len-1);
		ret = 0;
	}
	return ret;
}
