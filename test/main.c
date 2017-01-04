#include <stdio.h>
#include "../common/crc8.h"


int main()
{
	char szMsg[] = {"hello world!"};

	printf("%d\n", crc8(szMsg, strlen(szMsg)));
}
