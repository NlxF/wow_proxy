//
//  aidssl.c
//  wow_proxy
//
//  Created by luxiaofei on 2017/2/12.
//  Copyright © 2017年 luxiaofei. All rights reserved.
//

#ifdef SOCKSSL

#include "../../common/utility.h"
#include "aidssl.h"


/*---------------------------------------------------------------------*/
/*--- error_ssl - get the closest err mesage                        ---*/
/*---------------------------------------------------------------------*/
void error_ssl(char *msg, int size)
{
    u_long err;
    
    while ((err = ERR_get_error()) != 0)
    {
        ERR_error_string_n(err, msg, size);
    }
}


/*---------------------------------------------------------------------*/
/*--- init_server_ctx - initialize SSL server  and create context   ---*/
/*---------------------------------------------------------------------*/
SSL_CTX* init_server_ctx(void)
{
    SSL_METHOD *method;
    SSL_CTX *ctx;
    
    SSL_library_init();                 /* 初始化库，否则SSL_CTX_new会失败*/
    
    OpenSSL_add_all_algorithms();		/* load & register all cryptos, etc. */
    SSL_load_error_strings();			/* load all error messages */
    ERR_load_BIO_strings();
    
    /* 以 SSL V2 和 V3 标准兼容方式产生一个 SSL_CTX ，即 SSL Content Text */
    method = SSLv23_server_method();
    
    ctx = SSL_CTX_new(method);			/* create new context from method */
    if (ctx == NULL)
    {
        dbgprint("%s:%d:%s\n",__FILE__, __LINE__, "init ssl error");
    }
    
    return ctx;
}


/*---------------------------------------------------------------------*/
/*--- load_certificates - load from files.                          ---*/
/*---------------------------------------------------------------------*/
bool load_certificates(SSL_CTX* ctx, char *Cafile, char* CertFile, char* KeyFile)
{
    /* load the CA certificate from Cafile */
    if ( SSL_CTX_load_verify_locations(ctx, Cafile, NULL) <= 0)
    {
        dbgprint("%s:%d:%s:%s:%d\n", __FILE__, __LINE__, "varify the CA failed", Cafile, ERR_get_error());
        return false;
    }
    // allow this CA to be sent to the client during handshake
    if (SSL_CTX_use_certificate_chain_file(ctx, Cafile) <= 0)
    {
        dbgprint("%s:%d:%s\n", __FILE__, __LINE__, "Failed to add CA file into use certificate chain.");
        return false;
    }
    STACK_OF(X509_NAME) * list = SSL_load_client_CA_file(Cafile);
    if (NULL == list)
    {
        dbgprint("%s:%d:%s\n", __FILE__, __LINE__, "Failed to load SSL client CA file.");
        return false;
    }
    
//    SSL_CTX_set_client_CA_list(ctx, list);
    dbgprint("%s:%s\n", "client CA list", list);
    /* 设置证书密码 */
    //SSL_CTX_set_default_passwd_cb_userdata(ctx, "1234");
    
    /* set the local certificate from CertFile */
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        dbgprint("%s:%d:%s\n",__FILE__, __LINE__, "set the local certificate failed");
        return false;
    }
    /* set the private key from KeyFile */
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        dbgprint("%s:%d:%s\n",__FILE__, __LINE__, "set the private key failed");
        return false;
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        dbgprint("%s:%d:%s\n", __FILE__, __LINE__, "Private key does not match the public certificate");
        return false;
    }
    
    return true;
}


SSL_CTX *start_ssl(char *ca_file, char* cert_file, char* key_file, VerifyCallback verify_callback)
{
    //ssl variable
    SSL_CTX *ctx = NULL;
    
    ctx = init_server_ctx();                 /* initialize SSL */
    if (ctx == NULL)  return NULL;
    
    /* 载入用户的数字证书，此证书用来发送给客户端。证书里包含有公钥 */
    if(!load_certificates(ctx, ca_file, cert_file, key_file))   return NULL;
    
    //request client certificate
//    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_callback);
//    SSL_CTX_set_verify_depth(ctx, 1);
    
    dbgprint("init ssl...\n");
    
    return ctx;
}

/*---------------------------------------------------------------------*/
/*--- ShowCerts - print out certificates.                           ---*/
/*---------------------------------------------------------------------*/
void ShowCerts(SSL* ssl)
{
    X509 *cert;
    char *line;
    
    cert = SSL_get_peer_certificate(ssl);	/* Get certificates (if available) */
    if ( cert != NULL )
    {
        dbgprint("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        dbgprint("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        dbgprint("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        dbgprint("%s:%d:%s\n", __FILE__, __LINE__, "No certificates");
}

/*---------------------------------------------------------------------*/
/*--- shutdown_ssl - shutdown ssl.                                  ---*/
/*---------------------------------------------------------------------*/
void shutdown_ssl(SSL *ssl)
{
    /* 关闭 SSL 连接 */
    SSL_shutdown(ssl);
    
    /* 释放 SSL */
    SSL_free(ssl);
}


/*---------------------------------------------------------------------*/
/*--- destroy_ssl - destroy ssl.                                    ---*/
/*---------------------------------------------------------------------*/
void destroy_ssl(SSL_CTX *ctx)
{
    /* 释放 CTX */
    SSL_CTX_free(ctx);
    
    ERR_free_strings();
}

#endif
