/*
    This program is a "SOAP client" that makes a single "remote
    procedure call" to a web service at Google. The web service
    is a spell checking service.

    Notice that there is nothing like a "procedure call" abstraction
    in all of this code. What we see here is very much like any other
    HTTP request/response (but everything is in XML instead of HTML).
    The problem is that here we are doing all of the marshaling of the
    input ahd output parameters. This is what is usually done out of
    sight by the stub functions in an RPC system.
*/
#include <stdio.h>      // for fprintf() and fdopen()
#include <stdlib.h>     // for atoi() and exit()
#include <string.h>     // for memset(), bzero(), bcopy(), strerror(), strcat()
#include <errno.h>      // for errno

#include <unistd.h>     // for close()
#include <sys/socket.h> // for socket(), connect(), AF_INET, SOCK_STREAM
#include <arpa/inet.h>  // for sockaddr_in, htons()
#include <netdb.h>      // for gethostbyname(), hostent, hstrerror(), and h_errno


// prototypes for two error handling functions (defined below)
void  unix_error(char *msg);
void  dns_error(char *msg);


int main(int argc, char **argv)
{
    int socket_fd;                 // socket file descriptor
    struct hostent *hp;            // used to get server's IP address
    struct sockaddr_in serverAddr; // server address structure

    //char serverHostName[] = "127.0.0.1";
    char serverHostName[] = "192.168.1.115";
    char serverPortNumber[] = "7878";
    int requestLength, contentLength, countOut, countIn;
    #define MAXLINE 1024
    char request[MAXLINE];
    char soapEnvelope[MAXLINE];
    char buf[MAXLINE];

    // Create a reliable, stream socket using TCP.
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      unix_error("socket() error");


    // Construct a server address structure.
    bzero(&serverAddr, sizeof(serverAddr));        // zero out the address structure
    memset(&serverAddr, 0, sizeof(serverAddr));    // zero out the address structure
    serverAddr.sin_family = AF_INET;               // Internet address family
    if ( (hp = gethostbyname( serverHostName )) == NULL ) // fill in server's IP address
      dns_error("gethostbyname() error");
    bcopy(hp->h_addr, (struct sockaddr*)&serverAddr.sin_addr, hp->h_length);
    serverAddr.sin_port = htons( atoi(serverPortNumber) );  // fill in the port number

    // Establish a connection with the server.
    if (connect(socket_fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
      unix_error("connect() error");

    // the strings that make up the XML SOAP envelope
    char s0[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
    char s1[] = "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:ns1=\"urn:TC\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n";
    
    char s2[] = "  <SOAP-ENV:Body>\r\n";
    char s3[] = "    <ns1:executeCommand>\r\n";
    char s4[] = "      <command xsi:type=\"xsd:string\">pinfo tank2</command>\r\n";//account create abcde9 12345678
    char s5[] = "    </ns1:executeCommand>\r\n";
    char s6[] = "  </SOAP-ENV:Body>\r\n";
    char s7[] = "</SOAP-ENV:Envelope>\r\n";
    
    /*char s1[] = "<soap:Envelope xmlns:mrns0=\"urn:GoogleSearch\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:soapenc=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\r\n";
    char s2[] = "  <soap:Body soap:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n";
    char s3[] = "    <mrns0:doSpellingSuggestion>\r\n";
    char s4[] = "      <key xsi:type=\"xs:string\">NjdZ82tQFHIcJuozoEWdQjsL3Q3BNC7n</key>\r\n";
    char s5[] = "      <phrase xsi:type=\"xs:string\">britle</phrase>\r\n";
    char s6[] = "    </mrns0:doSpellingSuggestion>\r\n";
    char s7[] = "  </soap:Body>\n";
    char s8[] = "</soap:Envelope>\n";*/
    // "britle" could be bristle, brittle, bridle

    // concatenate these strings into a single string
    soapEnvelope[0] = '\0';
    strcat(soapEnvelope, s0);
    strcat(soapEnvelope, s1);
    strcat(soapEnvelope, s2);
    strcat(soapEnvelope, s3);
    strcat(soapEnvelope, s4);
    strcat(soapEnvelope, s5);
    strcat(soapEnvelope, s6);
    strcat(soapEnvelope, s7);
    //strcat(soapEnvelope, s8);
    contentLength = strlen(soapEnvelope);

    // prepare the request line and request headers
    request[0] = '\0';
    sprintf(request, "POST / HTTP/1.1\r\n");
    sprintf(request, "%sHost: 127.0.0.1:7878\r\n", request);
    sprintf(request, "%sContent-Type: text/xml; charset=UTF-8\r\n", request);
    sprintf(request, "%sContent-Length: %d\r\n", request, contentLength);
    sprintf(request, "%sSOAPAction: \"urn:TC#executeCommand\"\r\n", request);
    sprintf(request, "%sAuthorization: Basic YWRzOjEyMw==\"\r\n", request);
    sprintf(request, "%s\r\n", request);
    requestLength = strlen(request);

    int idx;
    for (idx=0; idx<2; idx++){
        
        // send the two strings to both the server and standard out
        countOut = write(socket_fd, request, requestLength);
        if (countOut != requestLength){
            unix_error("write() error");
        }
        countOut = write(socket_fd, soapEnvelope, contentLength);
        if (countOut != contentLength){
            unix_error("write() error");
        }
        printf("%s%s\n\n", request, soapEnvelope);
        
        // read in the response from the server and echo it to standard out
        while((countIn = read(socket_fd, buf, MAXLINE-1)) > 0){
            fputs(buf, stdout);
        }
        
        if (countIn <= 0){
            unix_error("read() error");
        }
    }

    // Close the socket.
    if (close(socket_fd) < 0){
        unix_error("close() error");
    }

    exit(0);
}//main


/*
   Below are two error handling functions.
   They provide reasonably meaningful error messages.
*/
void unix_error(char *msg)
{
   printf("%s: %s (errno = %d)\n", msg, strerror(errno), errno);
   exit(1);
}

void dns_error(char *msg)
{
   printf("%s: %s (h_errno = %d)\n", msg, hstrerror(h_errno), h_errno);
   exit(1);
}
