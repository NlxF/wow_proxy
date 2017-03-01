#include "aid_sql.h"

void test()
{
    if( init_commands_table() !=0 )
    {
        return -1;
    }
    dbgprint("init commands table successfully...\n");
    
    int i;
    for (i=0; i<15; i++)
    {
        dbgprint("search value for key: %d\n", i);
        
        elem em = value_for_key(i);
        
        if (em == NULL)
        {
            dbgprint("value for key %d not found...\n", i);
        }
        else
        {
            Command *cmd = em->info;
            dbgprint("value for key %d:\n", i);
            dbgprint("    command:%s\n", cmd->value);
            dbgprint("    needRsp:%s\n", cmd->needRsp?"True":"False");
            dbgprint("    deprecated:%s\n\n", cmd->deprecated?"True":"False");
        }
    }
}


int main ()
{
    test();
    
    return 0;
}
