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

bool analysis_response(char *rsp, size_t len, char **rst);

//#endif

#ifdef __cplusplus
}
#endif

#endif