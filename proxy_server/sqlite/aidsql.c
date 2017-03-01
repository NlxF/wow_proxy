#include <assert.h>
#include "sqlite3.h"
#include "config_.h"
#include "hashtable/hashtable.h"
#include "aidsql.h"


table command_table;    //hash table



static int callback_record(void *data, int argc, char **argv, char **azColName)
{
    
    /* create a elem */
    elem e = malloc(sizeof(struct elem));
    
    Command *cmd = malloc(sizeof(Command));
    
    int i;
    for(i=0; i<argc; i++)
    {
        if (strncmp(azColName[i], "key", 3)==0)
        {
            e->word = make_key(atoi(argv[i]));
        }
        else if (strncmp(azColName[i], "command", 7)==0)
        {
            strncpy(cmd->value, argv[i], strlen(argv[i]));
        }
        else if (strncmp(azColName[i], "needRsp", 7)==0)
        {
            cmd->needRsp = (bool)argv[i];
        }
        else if (strncmp(azColName[i], "deprecated", 10)==0)
        {
            cmd->deprecated = (bool)argv[i];
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
    
    char buf[MAX_SIZE] = {0};
    char db_path[MAX_SIZE] = {0};
    
    /* db */
    getcwd(buf, sizeof(buf));
    snprintf(db_path, "%s/%s", buf, "commands.db");
    
    sqlite3 *db;
    int rc = sqlite3_open(db_path, &db);
    if( rc )
    {
        dbgprint("%s:%d:%s: %s\n", __FILE__, __LINE__, "Can't open database", sqlite3_errmsg(db));
        return -1;
    }
    dbgprint("Opened database successfully\n");
    
    /* init hash table */
    int count = 0;
    char *zErrMsg = 0;
    rc = sqlite3_exec(db, "select count(*) from commands", callback_count, &count, &zErrMsg);
    if( rc != SQLITE_OK )
    {
        dbgprint("%s:%d:%s: %s\n", __FILE__, __LINE__, "SQL execute error", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -1;
    }
    
    command_table = table_new(count);
    if(!command_table)
    {
        dbgprint("%s:%d:%s\n", __FILE__, __LINE__, "create hash table failed\n");
        sqlite3_close(db);
        return -1;
    }
    
    /* Create SQL statement */
    char *sql = "SELECT * from commands";
    rc = sqlite3_exec(db, sql, callback_record, NULL/*(void*)data*/, &zErrMsg);
    if( rc != SQLITE_OK )
    {
        dbgprint("%s:%d:%s: %s\n", __FILE__, __LINE__, "SQL execute error", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -1;
    }
    
    dbgprint("Operation done successfully\n");
    
    sqlite3_close(db);
    
    return 0;
}



void *value_for_key(int key)
{
    if(command_table)
    {
        char* s = make_key(key);
        
        return ((elem)table_search(command_table, s))->info;
    }
    return NULL;
}
