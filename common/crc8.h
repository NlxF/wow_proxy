/*
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Very simple 8-bit CRC function.
 */

#ifndef _CRC8_H_
#define _CRC8_H_

#include <stdint.h>
#include <string.h>

//using x^8 + x^2 + x + 1 polynomial
#define POLY        0x07

uint8_t crc8(const void *data, int len);

int is_data_integrity(const void*data, int len);

/**
说明：将原始数据作CRC运算
参数：data:原始数据缓冲区
	  len:原始数据缓冲区长度
	  buf:结果缓冲区，长度为原始缓冲区长+1,(1为校验码)
返回值：成功返回buf长，失败返回-1
**/
int crc_data(const void* data, int len, void*buf);

/**
说明：将数据作逆CRC运算,返回原始数据,
参数：data:CRC数据缓冲区
	  len:CRC数据缓冲区长度
	  buf:原始数据缓冲区，长度为-1
返回值：成功返回0，失败返回-1
**/
int reverse_crc_data(const void *data, int len, void*buf);

#endif /* _CRC8_H_ */
