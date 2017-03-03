#include "aid_sql.h"

void test()
{
    if( init_commands_table() !=0 )
    {
        return;
    }
    dbgprint("init commands table successfully...\n");
    
    int i;
    for (i=0; i<15; i++)
    {
        printf("\n");
        dbgprint("search value for key: %d\n", i);
        
        Command* cmd = value_for_key(i);
        
        if (cmd == NULL)
        {
            dbgprint("value for key %d not found...\n", i);
        }
        else
        {
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
