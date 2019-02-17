#ifndef _ANALYSIS_SOAP_H_
#define _ANALYSIS_SOAP_H_

//
//  analysis_soap.h
//  wow_proxy
//
//  Created by luxiaofei on 2017/3/30.
//  Copyright © 2017年 luxiaofei. All rights reserved.
//

#include "../../common/config_.h"

#ifdef __cplusplus
extern "C" {
#endif

//#ifdef _SOAP

/**
 生成SOAP请求
 @param: 发送的内容
 @param: 内容长
 @param: 生成的请求
 @return: 请求的长
 */
size_t make_soap_request(char *content, size_t len, char *request);

    
/**
 解析SOAP格式的返回，提取result信息
 @param: rsp SOAP内容
 @param: len rsp长度
 @param: rst result信息返回或失败信息
 @param: rst数组长，返回时表示有效内容长
 @return NO:查询SOAP服务器失败,失败信息保存在rst返回;YES:查询成功
 */
bool analysis_soap_response(char *rsp, size_t len, char rst[], size_t *rst_len);

//#endif

#ifdef __cplusplus
}
#endif

#endif