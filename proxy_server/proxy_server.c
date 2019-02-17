#include "../common/config_.h"
#include "../common/utility.h"
#include "threadpool/threadpool.h"
#include "threadpool/aidsock.h"
#include "sqlite/aid_sql.h"
#ifdef SOCKSSL
#include "ssl/aidssl.h"
#endif

const char IMAGENAME[] = {"lserver:"};

threadpool *pthpool;    //thrad pool
bool  g_stop;

int  jobs = 0;          //For debug
int  loop = 0;          //For debug

#ifdef SOCKSSL
SSL_CTX* g_sslCtx;
BIO* errBio;
int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
    dbgprint(">>>> verifyCallback() - in: preverify_ok=%d\n", preverify_ok);
    
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
        
        dbgprint("Verify error: %s(%d)\n", X509_verify_cert_error_string(err), err);
        dbgprint(" - depth=%d\n", depth);
        dbgprint(" - sub  =\"%s\"\n", buf);
    }
    
    dbgprint("<<<< verifyCallback() - out\n");
    
    return preverify_ok;
}
#endif


void handle_accept(int ep_fd, int listen_fd)
{
    int  client_fd = 0;    //client fd

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
                dbgprint("%s:%d:handle accept client error%s\n",__FILE__, __LINE__, strerror(errno));
                break; //return -4;
            }
        }

        SOCKCONN *sockConn = malloc_sockconn(ep_fd, client_fd, EPOLLIN|EPOLLET|EPOLLRDHUP); //interest event
#ifdef SOCKSSL
        SSL *ssl = SSL_new(g_sslCtx);         					/* get new SSL state with context */
        if (ssl==NULL)
        {
            dbgprint("%s:%d:handle ssl accept(SSL_new) failed: %d\n",__FILE__, __LINE__, ERR_get_error());
            break;
        }
        sockConn->ssl = ssl;
        SSL_set_fd(ssl, client_fd);
        SSL_set_accept_state(ssl);
#endif
        //Make the incoming socket non-blocking and add it to the list of fds to monitor
        set_non_blocking(client_fd);
        
        //add epool fd
        add_epoll_fd(sockConn);
        dbgprint("handle accept %d, client fd=%d incoming\n", ++loop, sockConn->sock_fd);
    }
}

void handle_disconnect(SOCKCONN *sockConn)
{
    if (sockConn==NULL) return;

    dbgprint("free sockConn, with fd=%d, with address:%p\n", sockConn->sock_fd, sockConn);
    threadpool_add_work(pthpool, (void*)free_sockConn_func, (void*)sockConn);
}

void handle_read(SOCKCONN *sockConn, int container[2])
{
    if (sockConn==NULL) return;
    
    SOCKDATA *sockData = malloc_sockData(sockConn, container);
    if(sockData != NULL)
    {
        dbgprint("new read job, job number=%d, with fd=%d, with address:%p\n", ++jobs, sockConn->sock_fd, sockData);
        threadpool_add_work(pthpool, (void*)read_sock_func, (void*)sockData);
    }
}

void handle_write(SOCKCONN *sockConn)
{
    if (sockConn==NULL) return;
    
    int container[2] = {0};
    SOCKDATA *sockData = malloc_sockData(sockConn, container);
    if(sockData != NULL)
    {
        dbgprint("new write job, with fd=%d send data out\n", sockConn->sock_fd);
        threadpool_add_work(pthpool, (void*)write_sock_func, (void*)sockData);
    }
}

#ifdef SOCKSSL
void handle_handshake(SOCKCONN *sockConn) {
    
    int rtn = SSL_do_handshake(sockConn->ssl);
    if (rtn == 1)
    {
        sockConn->sslConnected = true;
        dbgprint("ssl connected success fd: %d\n", sockConn->sock_fd);
        return;
    }
    
    int err = SSL_get_error(sockConn->ssl, rtn);
    int old_evs = sockConn->events;
    if (err == SSL_ERROR_WANT_WRITE)                  //SSL需要在非阻塞socket可写时写入数据
    {
        sockConn->events |= EPOLLOUT;
        sockConn->events &= ~EPOLLIN;
        dbgprint("%s:%d:handle handshake return want write set events %d\n", __FILE__, __LINE__, sockConn->events);
    }
    else if (err == SSL_ERROR_WANT_READ)              //SSL需要在非阻塞socket可读时读入数据
    {
        sockConn->events |= EPOLLIN;
        sockConn->events &= ~EPOLLOUT;                //暂时不关注socket可写状态
        dbgprint("%s:%d:handle handshake return want read set events %d\n", __FILE__, __LINE__, sockConn->events);
    }
    else
    {
        dbgprint("%s:%d:handle handshake return %d error %d\n", __FILE__, __LINE__, rtn, err);
        ERR_print_errors(errBio);
        free_sockconn(sockConn);                      //关闭sock后，会自动从epoll轮询中移除
    }
    
    if (old_evs != sockConn->events)                  //更新关注事件
    {
        dbgprint("handle handshake update events from: %d to: %d\n", old_evs, sockConn->events);
        modify_epoll_fd(sockConn);
    }
}
#endif

static int pre_error = 0;   
static int error_num = 0;
void loop_once(int ep_fd, int listen_fd, int container[2])
{
    int    nfds;                             //ready I/O fd
    struct epoll_event event;
    struct epoll_event events[LISTEN_MAX];   //all events
    
    nfds = epoll_wait(ep_fd, events, LISTEN_MAX, -1);
    if(nfds == -1)
    {
        dbgprint("%s:%d:epoll_wait error:%d(%s)\n",__FILE__, __LINE__, errno, strerror(errno));
        if(errno == EINTR)
            return;                         // return and try again
        else
        {
            if(pre_error==errno)
                error_num++;
            else
                error_num = 0;
            if(error_num == ERRNUMTOEXIT)    // 若连续N次都是相同的错误
            {
                dbgprint("%s:%d:epoll_wait occur the error:%d(%s), %d times, something is wrong to exit!\n",__FILE__, __LINE__, errno, strerror(errno), ERRNUMTOEXIT);
                g_stop = true;
                return;
            }
            pre_error = errno;
        }
    }
    //handle all events
    int sock_num;
    for(sock_num=0; sock_num<nfds; ++sock_num)
    {
        SOCKCONN *sockConn = (SOCKCONN*)events[sock_num].data.ptr;
        int _event = events[sock_num].events;
        sockConn->events = _event;

        if((_event & EPOLLERR) || (_event & EPOLLHUP) /*||(!(eevents & EPOLLIN))*/)
        {
            //an error has occured on this fd
            dbgprint("%s:%d:An error:%d(%s) has occured on this fd:%d, or the socket is not ready for reading\n",__FILE__, __LINE__, errno, strerror(errno), sockConn->sock_fd);
            handle_disconnect(sockConn);
            continue;
        }
        else if((_event & EPOLLIN) && (_event & EPOLLRDHUP))
        {
            dbgprint("The remote has closed the connection of fd=%d\n", sockConn->sock_fd);
            handle_disconnect(sockConn);
            continue;
        }
        else if(_event & EPOLLIN)                   //data in include
        {
            if(sockConn->sock_fd == listen_fd)
            {
                // We have a notification on the listening socket, which means one or more incoming connections.
                handle_accept(ep_fd, listen_fd);
                continue;
            }
#ifdef SOCKSSL
            if (sockConn->sslConnected)
            {
#endif
                handle_read(sockConn, container);
                continue;
#ifdef SOCKSSL
            }
            handle_handshake(sockConn);
#endif
        }
        else if(_event & EPOLLOUT)                 //data out
        {
#ifdef SOCKSSL
            if(!sockConn->sslConnected)
            {
                handle_handshake(sockConn);
                continue;
            }
#endif
            handle_write(sockConn);
        }
        else
        {
            dbgprint("%s:%d:client: %d occur a unknown event", __FILE__, __LINE__, sockConn->sock_fd);
        }
    }
}

void handleInterrupt(int sig)
{
    g_stop = true;
}

int main(int argc, char*argv[])
{

    /* open syslog */
    openlog(IMAGENAME, LOG_CONS | LOG_PID, 0);
    
    /* ignore sigpipe */
    signal(SIGPIPE, SIG_IGN);
    
    /* handle stop interrupt */
    g_stop = false;
    signal(SIGINT, handleInterrupt);

    /* create threadpool */
    int thread_num = get_cpu_num();
    pthpool = threadpool_init(thread_num);
    if(pthpool == NULL)
    {
        return -1;
    }
    dbgprint("create threadpool...ok\n");
    
    
    int container[2] = {0};                 // embark write_fd、read_fd or soap_sock
#ifdef _SOAP
    /* init sock table */
    /* SOAP服务端在一次通信结束后会close, 所以不能复用socket  */
    /*
    table  soap_sock_table;                 //sock hash table
    soap_sock_table = init_soap_socket_table(pthpool->pthreads, thread_num);   //same num as thread
    if (!soap_sock_table)
    {
        return -1;
    }
    container[0] = soap_sock_table;
    */
#else
    /* redirect named pip */
    int  read_fd;                           //read fd
    int  write_fd;                          //write fd
    if(!redirect_namedpip(&write_fd, &read_fd))
    {
        return -1;
    }
    container[0] = read_fd;
    container[1] = write_fd;
#endif
    
    /* init command table */
    if( init_commands_table() !=0 )
    {
        return -1;
    }
    dbgprint("init commands table...ok\n");

    /* init request auth */
    if( init_request_auth() !=0 )
    {
        return -1;
    }
    dbgprint("init request auth...ok %s\n", fetch_request_auth());
    

#ifdef SOCKSSL
    char ca_file[MAX_BUF_SIZE]    = {0};
    char path_buf[MAX_BUF_SIZE]   = {0};
    char server_crt[MAX_BUF_SIZE] = {0};
    char server_key[MAX_BUF_SIZE] = {0};
    getcwd(path_buf,     sizeof(path_buf));
    snprintf(ca_file,    sizeof(ca_file),    "%s/%s", path_buf, "cert/cacert.pem");
    snprintf(server_crt, sizeof(server_crt), "%s/%s", path_buf, "cert/server_cert.pem");
    snprintf(server_key, sizeof(server_key), "%s/%s", path_buf, "cert/server_key.pem");
    
    g_sslCtx = start_ssl(ca_file, server_crt, server_key, verify_callback);
    if (g_sslCtx == NULL)
    {
        return -1;
    }
    errBio = BIO_new_fd(2, BIO_NOCLOSE);
#endif
    
    /* start socket */
    int listen_fd = create_server(SERVERPORT);
    if (listen_fd <= 0)
    {
        return -1;
    }
    
	dbgprint("start listening...\n");

    /* create epoll fd */
    int ep_fd = create_epoll(listen_fd);
    if(ep_fd < 0)
    {
        return -3;
    }
    
    SOCKCONN *sockConn = malloc_sockconn(ep_fd, listen_fd, EPOLLIN|EPOLLET);  //interest event
    
    add_epoll_fd(sockConn);
    
    /* start loop */
	dbgprint("start loop...\n");
    while(!g_stop)
    {
        loop_once(ep_fd, listen_fd, container);
    }
    
    close(listen_fd);
    free(sockConn);

    //free command table
    destory_commands_table();

    //free request auth
    destory_request_auth();

    //销毁线程池
    threadpool_destroy(pthpool);
#ifdef SOCKSSL
    SSL_CTX_free(g_sslCtx);
    BIO_free(errBio);
#endif

    closelog();
}
