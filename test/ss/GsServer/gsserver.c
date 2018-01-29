#include <stdio.h>
#include <stdlib.h>
#include "stdsoap2.h"


#include "sum.nsmap"
int total = 0;

int ns__sum(struct soap *sum_soap, int a, int b, int *res)
{
	// fprintf(stderr, "Total Request:%d\n", ++total);
	*res= a+ b;
	return 0;
}

int main(int argc, char* argv[])
{
	int m, s;
	struct soap sum_soap;
	soap_init(&sum_soap);
	// soap_init2(&sum_soap, SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE); 
	sum_soap.recv_timeout = 10;

	int sock_port = 18080;//listen port, sample!!!

	m = soap_bind(&sum_soap, NULL, sock_port, 100);
	if (m < 0) 
	{
		soap_print_fault(&sum_soap, stderr);
		exit(-1);
	}
	
	fprintf(stderr, "Socket Connection(listen socket = %d)\n", m);
	for ( ; ; ) 
	{
		s = soap_accept(&sum_soap);
		if (s < 0)
		{
			soap_print_fault(&sum_soap, stderr);
			exit(-1);
		}
		fprintf(stderr, "Socket Connection(new socket = %d) Total Request:%d\n", s, ++total);
		soap_serve(&sum_soap);
		soap_end(&sum_soap);
	}
	
	return 0;
}