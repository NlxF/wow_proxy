#ifndef _AIDSSL_H_
#define _AIDSSL_H_

//
//  aidssl.h
//  wow_proxy
//
//  Created by luxiaofei on 2017/2/12.
//  Copyright © 2017年 luxiaofei. All rights reserved.
//


#include "../../common/config_.h"

void error_ssl(char *msg, int size);

SSL_CTX *start_ssl(char *ca_file, char* cert_file, char* key_file, VerifyCallback erify_callback);

void ShowCerts(SSL* ssl);

void shutdown_ssl(SSL *ssl);

void destroy_ssl(SSL_CTX *ctx);


#endif
