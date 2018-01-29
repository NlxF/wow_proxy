/*
    HTTP/1.1 200 OK
    Server: gSOAP/2.8
    Content-Type: text/xml; charset=utf-8
    Content-Length: 864
    Connection: close

    <?xml version="1.0" encoding="UTF-8"?>
    <SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" xmlns:SOAP-ENC="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:ns1="urn:TC">
        <SOAP-ENV:Body>
            <ns1:executeCommandResponse>
                <result>
                    │Player  (offline) Tank (guid: 3)&#xD;&#xA;│ Account: TEST (ID: 1), GMLevel: 3&#xD;&#xA;│ Last Login: 2016-11-12 13:40:39 (Failed Logins: 0)&#xD;&#xA;│ OS: OSX - Latency: 0 ms&#xD;&#xA;└ Registration Email:  - Email: &#xD;&#xA;│ Last IP: 192.168.1.114 (Locked: No)&#xD;&#xA;│ Level: 80&#xD;&#xA;│ Race: Female 血精灵, 圣骑士&#xD;&#xA;│ Alive ?: Yes&#xD;&#xA;│ Money: 100000g0s0c&#xD;&#xA;│ Played time: 0h&#xD;&#xA;
                </result>
            </ns1:executeCommandResponse>
        </SOAP-ENV:Body>
    </SOAP-ENV:Envelope>

    出错情况下的返回：

    HTTP/1.1 500 Internal Server Error
    Server: gSOAP/2.8
    Content-Type: text/xml; charset=utf-8
    Content-Length: 556
    Connection: close

    <?xml version="1.0" encoding="UTF-8"?>
    <SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" xmlns:SOAP-ENC="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:ns1="urn:TC">
        <SOAP-ENV:Body>
            <SOAP-ENV:Fault>
                <faultcode>
                    SOAP-ENV:Client
                </faultcode>
                <faultstring>
                    An account with this name already exists!&#xD;&#xA;
                </faultstring>
                <detail>
                    An account with this name already exists!
                </detail>
            </SOAP-ENV:Fault>
        </SOAP-ENV:Body>
    </SOAP-ENV:Envelope>
*/
#include "xml.h"
#include "analysis_soap.h"

char *strnstr(s, find, slen)
	const char *s;
	const char *find;
	size_t slen;
{
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == '\0' || slen-- < 1)
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

bool partition_response(char *rsp, size_t len, char *http_header, char *soap)
{
    if (rsp==NULL||http_header==NULL||soap==NULL||len<=0)
        return false;
    
    char septation[] = "\r\n\r\n";
    char *pos = strnstr(rsp, septation, len);
    if(!pos)
        return false;
    
    strncpy(http_header, rsp, pos-rsp);                   //http header

    pos += strlen(septation);
    if(*pos=='<' && *(pos+1)=='?' && *(pos+2)=='x')      // remove "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    {
        pos = strnstr(pos, "?>", len);
        pos += 2;
    }
    strncpy(soap, pos, len);                             //soap content

    return true;
}


bool get_http_status(char *http_rsp, size_t len)
{
    if(http_rsp == NULL)
        return false;

    char *pos = strnstr(http_rsp, "200 OK", len);
    if(!pos)
        return false;
    else
        return true;
}


size_t get_soap_content(char *soap, bool isOK, char *content)
{
    if (soap==NULL||content==NULL)
        return -1;

    size_t content_len = 0;
    char szBuf[MAX_BUF_SIZE]={'\0'};
    strncpy(szBuf, soap, strlen(soap));

    struct xml_document* document = xml_parse_document(soap, strlen(soap));
    if (!document) 
    {
		printf("Could not parse document\n");
		return -1;
	}
	struct xml_node* root = xml_document_root(document);
    if(!root) return -1;
    struct xml_node* body_node = xml_node_child(root, 0);
    if(!body_node) return -1;

    if(isOK)
    {
        struct xml_node* rsp_node = xml_node_child(body_node, 0);
        if(!rsp_node) return -1;
        struct xml_node* rst_node = xml_node_child(rsp_node, 0);
        if(!rst_node) return -1;
        struct xml_string* node_name = xml_node_name(rst_node);
        if(!node_name) return -1;
        char* sznode_name = calloc(xml_string_length(node_name) + 1, sizeof(char));	
        xml_string_copy(node_name, sznode_name, xml_string_length(node_name));
        if(strncmp(sznode_name, "result", 6) == 0)
        {
            struct xml_string* node_content = xml_node_content(rst_node);
            content_len = xml_string_length(node_content);
            xml_string_copy(node_content, content, content_len);
        }
        free(sznode_name);
    }
    else
    {
        struct xml_node* fault_node = xml_node_child(body_node, 0);
        if(!fault_node) return -1;
        //try to get detail node 
        struct xml_node* detail_node = xml_node_child(fault_node, 2);
        if(!detail_node)
        {
            //get faultstring node
            struct xml_node* faultstring_node = xml_node_child(fault_node, 1);
            if(!faultstring_node) return -1;
            detail_node = faultstring_node;
        }
        struct xml_string* node_name = xml_node_name(detail_node);
        char* sznode_name = calloc(xml_string_length(node_name) + 1, sizeof(char));	
        xml_string_copy(node_name, sznode_name, xml_string_length(node_name));
        if(strncmp(sznode_name, "faultstring", 11)==0||strncmp(sznode_name, "detail", 6)==0)
        {
            struct xml_string* err_msg = xml_node_content(detail_node);
            content_len = xml_string_length(err_msg);
            xml_string_copy(err_msg, content, content_len);
        }

        free(sznode_name);
    }

    xml_document_free(document, false);

    return content_len;
}


bool analysis_soap_response(char *rsp, size_t rsp_len, char rst[], size_t *rst_len)
{
    if(rsp == NULL || *rst_len <= 0)
        return 0;
    
    bzero(rst, *rst_len);
    char soap[MAX_BUF_SIZE*3]={'\0'};
    char http_header[MAX_BUF_SIZE]={'\0'};

    if(!partition_response(rsp, rsp_len, http_header, soap))  //分离http头和soap content
        return 0;
    dbgprint("partition response...\n");
    // dbgprint("header:\n%s\n\ncontent:%s\n", http_header, soap);

    bool isOpOK;
    isOpOK = get_http_status(http_header, strlen(http_header));       //获取http头状态
    dbgprint("http header status=%s...\n", isOpOK?"OK":"Error");

    size_t size;
    char content[MAX_BUF_SIZE*4]={'\0'};
    size = get_soap_content(soap, isOpOK, content);     //获取soap content
    // dbgprint("soap content result:\n%s\n", content);
    if(size > 0 && size <= *rst_len)
    {
        strncpy(rst, content, size);
        *rst_len = size;
    }
    
    return isOpOK;
}


size_t make_soap_request(char *content, size_t len, char *soap_request)
{
    if (content==NULL || len<=0 || soap_request==NULL)
        return 0;
    
    int requestLength, contentLength;
    char request[MAX_BUF_SIZE*4] = {'\0'};
    char soapEnvelope[MAX_BUF_SIZE] = {'\0'};
    
    /*char s0[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
    char s1[] = "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:ns1=\"urn:TC\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n";
    char s2[] = "  <SOAP-ENV:Body>\r\n";
    char s3[] = "    <ns1:executeCommand>\r\n";
    char s4[] = "      <command xsi:type=\"xsd:string\">%s</command>\r\n";
    char s5[] = "    </ns1:executeCommand>\r\n";
    char s6[] = "  </SOAP-ENV:Body>\r\n";
    char s7[] = "</SOAP-ENV:Envelope>\r\n";

    char s8[MAX_BUF_SIZE] = {'\0'};
    sprintf(s8, s4, content);*/

    char s0[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
    char s1[] = "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:ns=\"urn:sum\">\r\n";
    char s2[] = "  <SOAP-ENV:Body SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n";
    char s3[] = "    <ns:sum>\r\n";
    char s4[] = "      <a>8</a>\r\n";
    char s5[] = "      <b>9</b>\r\n";
    char s6[] = "    </ns:sum>\r\n";
    char s7[] = "  </SOAP-ENV:Body>\r\n";
    char s8[] = "</SOAP-ENV:Envelope>\r\n";
    
    strcat(soapEnvelope, s0);
    strcat(soapEnvelope, s1);
    strcat(soapEnvelope, s2);
    strcat(soapEnvelope, s3);
    // strcat(soapEnvelope, s8);
    strcat(soapEnvelope, s4);
    strcat(soapEnvelope, s5);
    strcat(soapEnvelope, s6);
    strcat(soapEnvelope, s7);
    strcat(soapEnvelope, s8);
    contentLength = strlen(soapEnvelope);
    
    sprintf(request, "POST / HTTP/1.1\r\n");
    sprintf(request, "%sHost: 127.0.0.1:18000\r\n", request);
    sprintf(request, "%sUser-Agent: gSOAP/2.8\r\n", request);
    sprintf(request, "%sContent-Type: text/xml; charset=UTF-8\r\n", request);
    sprintf(request, "%sContent-Length: %d\r\n", request, contentLength);
    // sprintf(request, "%sSOAPAction: \"urn:TC#executeCommand\"\r\n", request);
    // sprintf(request, "%sAuthorization: Basic YWRzOjEyMw==\"\r\n", request);
    sprintf(request, "%s\r\n", request);
    requestLength = strlen(request);
    
    sprintf(soap_request, "%s%s", request, soapEnvelope);
    // dbgprint("make soap request:\n%s\n", soap_request);
    dbgprint("make soap request\n");
    
    return (contentLength+requestLength);
}






