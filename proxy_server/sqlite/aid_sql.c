#include <assert.h>
#include <unistd.h>
#include "aid_sql.h"


table command_table;    //hash table


static int callback_record(void *data, int argc, char **argv, char **azColName)
{
    
    /* create a elem */
    elem e = malloc(sizeof(struct elem));
    
    Command *cmd = malloc(sizeof(Command));
    memset(cmd, 0, sizeof(Command));
    
    int i;
//    dbgprint("\n");
//    dbgprint("Parse one row:\n");
    for(i=0; i<argc; i++)
    {
//        dbgprint("  %10s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        
        if (strncmp(azColName[i], "key", 3)==0)
        {
            if (argv[i])
                e->word = make_key_by_int(atoi(argv[i]));
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
    e->info = (void*)cmd;
    table_insert(command_table, e);
    
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
    
    char buf[MAX_BUF_SIZE]     = {0};
    char db_path[MAX_BUF_SIZE] = {0};
    
    /* db */
    getcwd(buf, sizeof(buf));
    snprintf(db_path, sizeof(db_path), "%s/%s", buf, "commands.db");
    
    
    sqlite3 *cmds_db;
    int rc = sqlite3_open(db_path, &cmds_db);
    if( rc )
    {
        dbgprint("%s:%d:%s: %s: %s\n", __FILE__, __LINE__, "Can't open database", sqlite3_errmsg(cmds_db), db_path);
        return -1;
    }
    //dbgprint("Opened database successfully\n");
    
    /* init hash table */
    int count = 0;
    char *zErrMsg = 0;
    rc = sqlite3_exec(cmds_db, "select count(*) from commands", callback_count, &count, &zErrMsg);
    if( rc != SQLITE_OK )
    {
        dbgprint("%s:%d:%s: %s\n", __FILE__, __LINE__, "SQL execute error", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(cmds_db);
        return -1;
    }
    
    command_table = table_new( count/3 );   //fix size
    if(!command_table)
    {
        dbgprint("%s:%d:%s\n", __FILE__, __LINE__, "create hash table failed\n");
        sqlite3_close(cmds_db);
        return -1;
    }
    
    /* Create SQL statement */
    char *sql = "SELECT * from commands";
    rc = sqlite3_exec(cmds_db, sql, callback_record, NULL/*(void*)data*/, &zErrMsg);
    if( rc != SQLITE_OK )
    {
        dbgprint("%s:%d:%s: %s\n", __FILE__, __LINE__, "SQL execute error", zErrMsg);
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
        char* s = make_key_by_int(key);
        elem em = (elem)table_search(command_table, s);
        if (em)
        {
            return (Command*)em->info;
        }
    }
    return NULL;
}


void destory_commands_table()
{
    if (command_table)
    {
        table_free(command_table);
    }
}








