#include "../common/config_.h"
#include "../common/utility.h"
#include "threadpool/threadpool.h"
#include "threadpool/aidsock.h"
#ifdef SOCKSSL
#include "ssl/aidssl.h"
#endif

const char IMAGENAME[] = {"lserver:"};
threadpool *pthpool;                                 //thrad pool
SSL_CTX* g_sslCtx;

bool redirect_namedpip(int *write_fd, int *read_fd){

    char l2w[MAX_SIZE] ={0};
    char w2l[MAX_SIZE] = {0};
    char current_abs_path[MAX_SIZE] ={0};
    
    //get_absolute_path(current_abs_path);
    get_home_Path(current_abs_path, sizeof(current_abs_path));
    snprintf(l2w, MAX_SIZE, "%s/%s", current_abs_path,LSERVER2WSERVER);
    snprintf(w2l, MAX_SIZE, "%s/%s", current_abs_path,WSERVER2LSERVER);
    if((mkfifo(l2w, 0666)==-1&&errno!=EEXIST)||(mkfifo(w2l,0666)==-1&&errno!=EEXIST))
    {
        dbgprint("%s:%d:%s\n", __FILE__, __LINE__,strerror(errno));
        return false;
    }
    *write_fd = open(l2w, O_RDWR);
    *read_fd = open(w2l, O_RDWR);
    if(*write_fd==-1 || *read_fd==-1)
    {
        dbgprint("%s:%d:%s\n",__FILE__, __LINE__, "open pipe error!");
        return false;
    }
    
    dbgprint("open pipe....\n");
    
    //set process max fd, 0 for RLIM_INFINITY,
    set_process_max_fd(0);
    
    return true;
}

int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
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

typedef  int (*VerifyCallback)(int, X509_STORE_CTX *);
SSL_CTX *start_ssl(VerifyCallback erify_callback)
{
    //ssl variable
    SSL_CTX *ctx = NULL;
    
    ctx = init_server_ctx();                 /* initialize SSL */
    if (ctx == NULL)
    {
        dbgprint("%s:%d:%s\n",__FILE__, __LINE__, "init ssl error");
        return NULL;
    }
    /* 载入用户的数字证书，此证书用来发送给客户端。证书里包含有公钥 */
    char szPath[MAX_SIZE] = {0};
    char CA_file[MAX_SIZE] = {0};
    char server_crt[MAX_SIZE] = {0};
    char server_key[MAX_SIZE] = {0};
    getcwd(szPath, sizeof(szPath));
    snprintf(CA_file,    sizeof(CA_file),    "%s/%s", szPath, "cert/cacert.pem");
    snprintf(server_crt, sizeof(server_crt), "%s/%s", szPath, "cert/server_crt.pem");
    snprintf(server_key, sizeof(server_key), "%s/%s", szPath, "cert/server_key.pem");
    if(!load_certificates(ctx, CA_file, server_crt, server_key))
    {
        return NULL;
    }
    
    //request client certificate
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_callback);
    
    dbgprint("init ssl...\n");
    
    g_sslCtx = ctx;
    return ctx;
}


int start_socket(int port)
{

    if(port <= 0) return -1;
    
    int  listen_fd;
    
    //create listening socket
    if((listen_fd=create_listen_sock(port)) < 0)
        return -1;
    dbgprint("create listen sock...\n");
    
    //begin listening
    if(listen(listen_fd, LISTEN_MAX) == -1)
    {
        dbgprint("%s:%d:listen sock error:%s\n",__FILE__, __LINE__, strerror(errno));
        return -2;
    }
    
    return listen_fd;
    
}

int  loop = 0;

void add_epoll_fd(SOCKCONN *sockConn)
{
    if(sockConn==NULL)  return;
    
    struct epoll_event event;
    
//    event.data.fd = sockConn.client_fd;
    event.data.ptr = sockConn
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(sockConn.ep_fd, EPOLL_CTL_ADD, sockConn.client_fd, &event);      //add new event
    
    dbgprint("%d accept client :%d\n", ++loop, client_fd);
}

void handle_accept(int ep_fd, int listen_fd)
{
    int  client_fd;    //client fd

    while(1)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);
        if(client_fd == -1)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                //We have processed all incoming connections.
                break;
            }
            else
            {
                dbgprint("%s:%d:accept client error%s\n",__FILE__, __LINE__, strerror(errno));
                break; //return -4;
            }
        }
        
        SOCKCONN *sockConn = (SOCKCONN*)malloc(sizeof(SOCKCONN));
        memset(sockConn, 0, sizeof(SOCKCONN));
        sockConn.ep_fd = ep_fd;
        sockConn.client_fd = client_fd;
#ifdef SOCKSSL
        SSL *ssl = SSL_new(g_sslCtx);         					/* get new SSL state with context */
        if (ssl==NULL)
        {
            char szMsg[MAX_SIZE] = {0};
            error_ssl(szMsg, sizeof(szMsg));
            dbgprint("%s:%d:SSL_new failed%s\n",__FILE__, __LINE__, szMsg);
            break;
        }
        sockConn.ssl = ssl;
        SSL_set_fd(ssl, client_fd);
        SSL_set_accept_state(ssl);
#endif
        //Make the incoming socket non-blocking and add it to the list of fds to monitor
        set_non_blocking(client_fd);
        
        //add epool fd
        add_epoll_fd(sockConn);
    }
}

void handle_read(int sock_fd, int read_fd, int write_fd)
{
    if (sock_fd<=0||read_fd<=0||write_fd<=0) return;
    
    SOCKDATA *pdata = (SOCKDATA*)malloc(sizeof(SOCKDATA));
    bzero(pdata, sizeof(SOCKDATA));
    pdata->sock_fd = sock_fd;
    pdata->write_fd = write_fd;
    pdata->read_fd = read_fd;
    threadpool_add_work(pthpool, (void*)read_sock, (void*)pdata);
}

void handle_write(int sock_fd)
{
    if (sock_fd<=0) return;
    
    SOCKDATA *pdata = (SOCKDATA*)malloc(sizeof(SOCKDATA));
    bzero(pdata, sizeof(SOCKDATA));
    pdata->sock_fd = sock_fd;
    threadpool_add_work(pthpool, (void*)write_sock, (void*)pdata);
}

void handle_handshake(Channel* ch) {
    if (!ch->tcpConnected_) {
        struct pollfd pfd;
        pfd.fd = ch->fd_;
        pfd.events = POLLOUT | POLLERR;
        int r = poll(&pfd, 1, 0);
        if (r == 1 && pfd.revents == POLLOUT) {
            log("tcp connected fd %d\n", ch->fd_);
            ch->tcpConnected_ = true;
            ch->events_ = EPOLLIN | EPOLLOUT | EPOLLERR;
            ch->update();
        } else {
            log("poll fd %d return %d revents %d\n", ch->fd_, r, pfd.revents);
            delete ch;
            return;
        }
    }
    if (ch->ssl_ == NULL) {
        ch->ssl_ = SSL_new (g_sslCtx);
        check0(ch->ssl_ == NULL, "SSL_new failed");
        int r = SSL_set_fd(ch->ssl_, ch->fd_);
        check0(!r, "SSL_set_fd failed");
        log("SSL_set_accept_state for fd %d\n", ch->fd_);
        SSL_set_accept_state(ch->ssl_);
    }
    int r = SSL_do_handshake(ch->ssl_);
    if (r == 1) {
        ch->sslConnected_ = true;
        log("ssl connected fd %d\n", ch->fd_);
        return;
    }
    int err = SSL_get_error(ch->ssl_, r);
    int oldev = ch->events_;
    if (err == SSL_ERROR_WANT_WRITE) {
        ch->events_ |= EPOLLOUT;
        ch->events_ &= ~EPOLLIN;
        log("return want write set events %d\n", ch->events_);
        if (oldev == ch->events_) return;
        ch->update();
    } else if (err == SSL_ERROR_WANT_READ) {
        ch->events_ |= EPOLLIN;
        ch->events_ &= ~EPOLLOUT;
        log("return want read set events %d\n", ch->events_);
        if (oldev == ch->events_) return;
        ch->update();
    } else {
        log("SSL_do_handshake return %d error %d errno %d msg %s\n", r, err, errno, strerror(errno));
        ERR_print_errors(errBio);
        delete ch;
    }
}

bool    g_stop = false;
int     jobs = 0;

void loop_once(int ep_fd, int listen_fd, int read_fd, int write_fd)
{
    int    nfds;         //ready I/O fd
    int    client_num;
    struct epoll_event event;
    struct epoll_event events[LISTEN_MAX];   //all events
    
    nfds = epoll_wait(ep_fd, events, LISTEN_MAX, -1);
    
    //handle all events
    for(client_num=0; client_num<nfds; ++client_num)
    {
        CSOCKCONN *sockConn = (CSOCKCONN*)events[i].data.ptr;
        
        if((events[client_num].events & EPOLLERR) ||
           (events[client_num].events & EPOLLHUP) /*||
           (!(events[client_num].events & EPOLLIN))*/)
        {
            //an error has occured on this fd
            dbgprint("%s:%d:An error:%d(%s) has occured on this fd:%d, or the socket is not ready for reading%s\n",__FILE__, __LINE__, errno, strerror(errno), events[client_num].data.fd);
            
            close(events[client_num].data.fd);
            continue;
        }
        else if(events[client_num].data.fd == listen_fd)
        {
            // We have a notification on the listening socket, which means one or more incoming connections.
            handle_accept(ep_fd, listen_fd);
            continue;
        }
        else if(events[client_num].events & EPOLLIN)              //data in
        {
            handle_read(events[client_num].data.fd, read_fd, write_fd);
            dbgprint("job number :%d, with client: %d\n", ++jobs, events[client_num].data.fd);
        }
        else if(events[client_num].events & EPOLLOUT)             //data out
        {
            handle_write(events[client_num].data.fd)
            dbgprint("client: %d send data out", events[client_num].data.fd);
        }else
        {
            
        }
    }
}

void handleInterrupt(int sig)
{
    g_stop = true;
}

int main(int argc, char*argv[])
{
    
    int  read_fd;      //read fd
    int  write_fd;     //write fd
    
    signal(SIGINT, handleInterrupt);
    
    //open syslog
    openlog(IMAGENAME, LOG_CONS | LOG_PID, 0);

    //redirect named pip
    if(!redirect_namedpip(&write_fd, &read_fd))
    {
        return -1;
    }

    //create threadpool
    pthpool = threadpool_init(get_cpu_num());
    if(pthpool == NULL)
    {
        dbgprint("%s:%d:%s\n",__FILE__, __LINE__, "create threadpool error");
        return -1;
    }
	dbgprint("create threadpool...\n");

#ifdef SOCKSSL
    //start ssl
    if (start_ssl() == NULL)
    {
        dbgprint("%s:%d:%s\n",__FILE__, __LINE__, "init ssl error");
        return -1;
    }
#endif
    
    //start socket
    int listen_fd = start_socket(8083);
    if (listen_fd <= 0)
    {
        return -1;
    }
	dbgprint("start listening...\n");

    //create epoll fd
    int ep_fd = create_epoll(listen_fd);
    if(ep_fd < 0)
    {
        dbgprint("%s:%d:create epoll error:%s\n",__FILE__, __LINE__, strerror(errno));
        return -3;
    }
    
    //start loop
	dbgprint("start loop...\n");
    while(!g_stop)
    {
        loop_once(ep_fd, listen_fd, read_fd, write_fd);
    }
    

    close(listen_fd);
    
#ifdef SOCKSSL
    SSL_CTX_free(ctx);
#endif
    
    closelog();
    
}
