#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <lua.h>


#define LSERVER2WSERVER "/.lserver2wserver"
#define WSERVER2LSERVER "/.wserver2lserver"
#define MAX_SIZE 1024

int getPipPath(char szPath[512], int nbyte)
{
    bzero(szPath, nbyte);
    
    char *home_path = getenv("HOME");
    sprintf(szPath, "%s/%s", home_path, ".wowserver");
    mkdir(szPath, 0777);
    if(errno != 0 && errno != 17)
    {
        printf("error:%d = %s\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

int RedirectToPip()
{
    //printf("It will redirect to pip for automation");
    char szpath[512];
    if(getPipPath(szpath, sizeof(szpath)) != 0)
        return -1;
    
    char l2w[MAX_SIZE] ={0};
    char w2l[MAX_SIZE] = {0};
    
    snprintf(l2w, MAX_SIZE, "%s/%s", szpath, LSERVER2WSERVER);
    snprintf(w2l, MAX_SIZE, "%s/%s", szpath, WSERVER2LSERVER);
    if((mkfifo(l2w, 0666)==-1&&errno!=EEXIST)||(mkfifo(w2l,0666)==-1&&errno!=EEXIST))
    {
        printf("%s:%d:%s\n", __FILE__, __LINE__,strerror(errno));
        unlink(l2w);
        unlink(w2l);
        return -2;
    }
    
//    int write_fd = open(w2l, O_RDWR);
    int read_fd = open(l2w, O_RDWR);
    if(/*write_fd==-1||*/read_fd==-1)
    {
        printf("%s:%d:%s\n",__FILE__, __LINE__, "open pipe error!");
        return -3;
    }
    
//    dup2(write_fd, STDOUT_FILENO);
    dup2(read_fd, STDIN_FILENO);
    
    return 1;
}

int init_lua_env()
{
    printf("** Init Lua\n");
    lua_State *L;
    L = luaL_newstate();
    printf("** Load the (optional) standard libraries, to have the print function\n");
    luaL_openlibs(L);
    printf("** Load chunk. without executing it\n");
    if (luaL_loadfile(L, "luascript.lua"))
    {
        printf("Something went wrong loading the chunk (syntax error?)\n");
        printf("%s\n", lua_tostring(L, -1));
        lua_pop(L,1);
        return -1;
    }
    
    printf("** Make a insert a global var into Lua from C\n");
    lua_pushstring(L, "hello world!!!");
    lua_setglobal(L, "cppvar");
    
    printf("** Execute the Lua chunk\n");
    if (lua_pcall(L,0, LUA_MULTRET, 0)) {
        printf("Something went wrong during execution\n");
        printf("%s\n", lua_tostring(L, -1));
        lua_pop(L,1);
        return -1;
    }
    
    printf("** Release the Lua enviroment\n");
    lua_close(L);
    return 0;
}

int exec_lua_again()
{
    printf("** Init Lua\n");
    lua_State *L;
    L = luaL_newstate();
    printf("** Load the (optional) standard libraries, to have the print function\n");
    luaL_openlibs(L);
    printf("** Load chunk. without executing it\n");
    if (luaL_loadfile(L, "luascript.lua"))
    {
        printf("Something went wrong loading the chunk (syntax error?)\n");
        printf("%s\n", lua_tostring(L, -1));
        lua_pop(L,1);
        return -1;
    }
    
    printf("** Execute the Lua chunk\n");
    if (lua_pcall(L,0, LUA_MULTRET, 0)) {
        printf("Something went wrong during execution\n");
        printf("%s\n", lua_tostring(L, -1));
        lua_pop(L,1);
        return -1;
    }
    
    printf("** Release the Lua enviroment\n");
    lua_close(L);
    return 0;
}

int main()
{
	//redirection to pipe
    if(RedirectToPip()<0)
    {
        printf("redirect to pip failed\n");
        return;
    }
    printf("redirect to pip...\n");
    
//    if(init_lua_env()<0)
//    {
//        printf("init lua env failed\n");
//        return;
//    }
//    printf("init lua env...\n");
//    
//    exec_lua_again();
    
	while(1)
	{
		time_t t;
        
		//int nbytes = read(read_fd, data.msg, 1024);
        printf("read fron stdin...\n");
		char *command_str = readline(NULL);
		
		time(&t);
		printf("\n-----------------\n");
		printf("%sRecv: %s\n", asctime(gmtime(&t)), command_str);

		//printf(data.msg);
		//printf("\n-----------------\n");		
	}
	printf("end read/write\n");
}
