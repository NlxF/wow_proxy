#include <stdio.h>
#include <stdlib.h>

int main()
{
    char szpath[512] = {0};
    
    sprintf(szpath, "%s=%s\n", "key", "123");
    
    printf(szpath);
    
}
