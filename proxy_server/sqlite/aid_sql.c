#include <assert.h>
#include <unistd.h>
#include "aid_sql.h"


// table command_table;    
Table *command_table;   //hash table

static int callback_record(void *data, int argc, char **argv, char **azColName)
{ 
    /* create a elem */
    Table *elem = malloc(sizeof(Table));
    memset(elem, 0, sizeof(Table));
    Command *cmd = &(elem->cmd);

//    dbgprint("\n");
//    dbgprint("Parse one row:\n");
    int i;
    for(i=0; i<argc; i++)
    {
    //    dbgprint("  %10s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        
        if (strncmp(azColName[i], "key", 3)==0)
        {
            if (argv[i])
                elem->command_id = atoi(argv[i]);
        }
        else if (strncmp(azColName[i], "command", 7)==0)
        {
            if (argv[i])
                strncpy(cmd->value, argv[i], strlen(argv[i]));
        }
        else if (strncmp(azColName[i], "needRsp", 7)==0)
        {
            if (argv[i])
                cmd->needRsp = atoi(argv[i]);
        }
        else if (strncmp(azColName[i], "deprecated", 10)==0)
        {
            if (argv[i])
                cmd->deprecated = atoi(argv[i]);
        }
        else if (strncmp(azColName[i], "num", 3)==0)
        {
            if (argv[i])
                cmd->paramNum = atoi(argv[i]);
        }
        else if (strncmp(azColName[i], "pipe", 4)==0)
        {
            if (argv[i])
                cmd->is2Pipe = atoi(argv[i]);
        }
    }
    
    HASH_ADD_INT(command_table, command_id, elem); 
    elem = NULL;
    
    return 0;
}

static int callback_count(void *count, int argc, char **argv, char **azColName)
{
    int *c = count;
    *c = atoi(argv[0]);
    return 0;
}

int init_commands_table()
{
    
    char szBuf[MAX_BUF_SIZE]     = {0};
    char db_path[MAX_BUF_SIZE]   = {0};
    
    /* db */
    getcwd(szBuf, sizeof(szBuf));
    snprintf(db_path, sizeof(db_path), "%s/%s", szBuf, "commands.db");
    
    sqlite3 *cmds_db = NULL;
    int rc = sqlite3_open(db_path, &cmds_db);
    if( rc )
    {
        // dbgprint("%s:%d:%s: %s: %s\n", __FILE__, __LINE__, "Can't open database", sqlite3_errmsg(cmds_db), db_path);
        return -1;
    }
    //dbgprint("Opened database successfully\n");
    
    /* Get row count */
    // int count = 0;
    // char *zErrMsg = 0;
    // rc = sqlite3_exec(cmds_db, "select count(*) from commands", callback_count, &count, &zErrMsg);
    // if( rc != SQLITE_OK )
    // {
    //     dbgprint("%s:%d:%s: %s\n", __FILE__, __LINE__, "SQL execute error", zErrMsg);
    //     sqlite3_free(zErrMsg);
    //     sqlite3_close(cmds_db);
    //     return -1;
    // }

    
    /* Create SQL statement */
    char *zErrMsg = 0;
    char *sql = "SELECT * from commands";
    rc = sqlite3_exec(cmds_db, sql, callback_record, NULL/*(void*)data*/, &zErrMsg);
    if( rc != SQLITE_OK )
    {
        // dbgprint("%s:%d:%s: %s\n", __FILE__, __LINE__, "SQL execute error", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(cmds_db);
        return -1;
    }
    
    sqlite3_close(cmds_db);
    
    return 0;
}


Command *value_for_key(int key)
{
    if(command_table)
    {
        dbgprint("search value for key:%d.\n", key);
        Table *elem;
        HASH_FIND_INT(command_table, &key, elem);
        if(elem)
        {
            // dbgprint("Find command:[value:%s, paramNum:%d, needRsp:%d, deprecated:%d, is2Pipe:%d] for key:%d\n", elem->cmd.value, elem->cmd.paramNum, elem->cmd.needRsp, elem->cmd.deprecated, elem->cmd.is2Pipe, key);
            return &(elem->cmd);
        }
    }
    return NULL;
}


void destory_commands_table()
{
    Table *tmp          = NULL;
    Table *current_elem = NULL;

    HASH_ITER(hh, command_table, current_elem, tmp)
    {
        HASH_DEL(command_table, current_elem);
        free(current_elem);
    }
}








