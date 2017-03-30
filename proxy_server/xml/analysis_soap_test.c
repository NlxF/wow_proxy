#include "analysis_soap.h"

int main()
{
    char *source = "HTTP/1.1 500 Internal Server Error"
                   "Server: gSOAP/2.8"
                    "Content-Type: text/xml; charset=utf-8"
                    "Content-Length: 556"
                    "Connection: close"
                    "\r\n\r\n"
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                    "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:ns1=\"urn:TC\">"
                        "<SOAP-ENV:Body>"
                            "<SOAP-ENV:Fault>"
                                "<faultcode>"
                                    "SOAP-ENV:Client"
                                "</faultcode>"
                                "<faultstring>"
                                    "An account with this name already exists!&#xD;&#xA;"
                                "</faultstring>"
                                "<detail>"
                                    "An account with this name already exists!"
                                "</detail>"
                            "</SOAP-ENV:Fault>"
                        "</SOAP-ENV:Body>"
                    "</SOAP-ENV:Envelope>";
    
    char *rst = NULL;

    analysis_response(source, strlen(source), &rst);
    
    printf(rst);

    free(rst);
}
