/* ssl_client.c
 *
 * Copyright (c) 2000 Sean Walton and Macmillan Publishers.  Use may be in
 * whole or in part in accordance to the General Public License (GPL).
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*****************************************************************************/
/*** ssl_client.c                                                          ***/
/***                                                                       ***/
/*** Demonstrate an SSL client.                                            ***/
/*****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define FAIL    -1
#define MAX_VERIFY_DEPTH 1

/*---------------------------------------------------------------------*/
/*--- OpenConnection - create socket and connect to server.         ---*/
/*---------------------------------------------------------------------*/
int OpenConnection(const char *hostname, int port)
{   int sd;
    struct hostent *host;
    struct sockaddr_in addr;
    
    if ( (host = gethostbyname(hostname)) == NULL )
    {
        perror(hostname);
        abort();
    }
    sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = *(long*)(host->h_addr);
    if ( connect(sd, &addr, sizeof(addr)) != 0 )
    {
        close(sd);
        perror(hostname);
        abort();
    }
    return sd;
}


int verifyCallback(int preverify_ok, X509_STORE_CTX *ctx)
{
    printf(">>>> verifyCallback() - in: preverify_ok=%d\n", preverify_ok);
    
    if(!preverify_ok)
    {
        char buf[256];
        X509 *err_cert;
        int err, depth;
        SSL *ssl;
        
        err_cert = X509_STORE_CTX_get_current_cert(ctx);
        err = X509_STORE_CTX_get_error(ctx);
        depth = X509_STORE_CTX_get_error_depth(ctx);
        ssl = (SSL*)X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
        X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);
        
        printf("Verify error: %s(%d)\n", X509_verify_cert_error_string(err), err);
        printf(" - depth=%d\n", depth);
        printf(" - sub  =\"%s\"\n", buf);
    }
    
    printf("<<<< verifyCallback() - out\n");
    //return 1; 
    return preverify_ok; 
}


/*---------------------------------------------------------------------*/
/*--- InitCTX - initialize the SSL engine.                          ---*/
/*---------------------------------------------------------------------*/
SSL_CTX* InitCTX(void)
{
    SSL_METHOD *method;
    SSL_CTX *ctx;
    
    SSL_library_init();
    
    OpenSSL_add_all_algorithms();		/* Load cryptos, et.al. */
    SSL_load_error_strings();			/* Bring in and register error messages */
    method = SSLv23_client_method();	/* Create new client-method instance */
    ctx = SSL_CTX_new(method);			/* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

/*---------------------------------------------------------------------*/
/*--- Added the LoadCertificates how in the server-side makes.      ---*/
/*---------------------------------------------------------------------*/
void LoadCertificates(SSL_CTX* ctx, char* Cafile, char* CertFile, char* KeyFile)
{
    /* load the CA certificate from Cafile */
    if ( SSL_CTX_load_verify_locations(ctx, Cafile, NULL) <= 0)
    {
        printf("varify the CA:%s failed\n", Cafile);
        abort();
    }
    /* set the local certificate from CertFile */
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        printf("Private key does not match the public certificate\n");
        abort();
    }
}

/*---------------------------------------------------------------------*/
/*--- ShowCerts - print out the certificates.                       ---*/
/*---------------------------------------------------------------------*/
void ShowCerts(SSL* ssl)
{   X509 *cert;
    char *line;
    
    cert = SSL_get_peer_certificate(ssl);	/* get the server's certificate */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);							/* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);							/* free the malloc'ed string */
        X509_free(cert);					/* free the malloc'ed certificate copy */
    }
    else
        printf("No certificates.\n");
}

/*---------------------------------------------------------------------*/
/*--- main - create SSL context and connect                         ---*/
/*---------------------------------------------------------------------*/
int main(int count, char *strings[])
{
    SSL_CTX *ctx;
    int server;
    SSL *ssl;
    char buf[1024];
    int bytes;
    char *hostname, *portnum;
    char CertFile[1024];
    char KeyFile[1024];
    char caFile[1024];
    
    hostname = "127.0.0.1";
    portnum = "8083";
    
    getcwd(buf, sizeof(buf));
    snprintf(CertFile, sizeof(CertFile), "%s/%s", buf, "cert/client_cert.pem");
    snprintf(KeyFile,  sizeof(KeyFile),  "%s/%s", buf, "cert/client_key.pem");
    snprintf(caFile,   sizeof(caFile) ,  "%s/%s", buf, "cert/cacert.pem");
    
    ctx = InitCTX();
    LoadCertificates(ctx, caFile, CertFile, KeyFile);
//    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verifyCallback);
//    SSL_CTX_set_verify_depth(ctx, MAX_VERIFY_DEPTH);
    server = OpenConnection(hostname, atoi(portnum));
    ssl = SSL_new(ctx);						/* create new SSL connection state */
    SSL_set_fd(ssl, server);				/* attach the socket descriptor */
    if ( SSL_connect(ssl) == FAIL )			/* perform the connection */
    {
//        ERR_print_errors_fp(stderr);
        printf("connect failed\n");
    }
    else
    {   char *msg = "Hello world";
        
//        if(SSL_get_verify_result(ssl)!=X509_V_OK)
//        {
//            printf("SSL verify failed: ");
//            ERR_print_errors_fp(stderr);
//            abort();
//        }
//        printf("SSL handshake/verify successful\n");
        
        printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
        
        ShowCerts(ssl);								/* get any certs */
        
        SSL_write(ssl, msg, strlen(msg));			/* encrypt & send message */
        printf("Send: %s\n", msg);
        bytes = 1;
        while(bytes)
        {
            bytes = SSL_read(ssl, buf, sizeof(buf));	/* get reply & decrypt */
            buf[bytes] = '\0';
            printf("Received: \"%s\"\n", buf);
        }
        SSL_shutdown(ssl);
        SSL_free(ssl);								/* release connection state */
    }
    close(server);									/* close socket */
    SSL_CTX_free(ctx);								/* release context */
}
