//
//  aidssl.h
//  wow_proxy
//
//  Created by luxiaofei on 2017/2/12.
//  Copyright © 2017年 luxiaofei. All rights reserved.
//


#include "../../common/config_.h"

void error_ssl(char *msg, int size);

SSL_CTX* init_server_ctx(void);

bool load_certificates(SSL_CTX* ctx, char *Cafile, char* CertFile, char* KeyFile);

void ShowCerts(SSL* ssl);

void shutdown_ssl(SSL *ssl);

void destroy_ssl(SSL_CTX *ctx);


