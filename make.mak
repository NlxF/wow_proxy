; 指明目标格式：exe, lib, dll 三选一
mode: exe

; 编译选项
flag: -g, -Wall, -O2, -DNDEBUG
flag: -g, -Wall, -O0, -DDEBUG

; 设定链接
link: stdc++
link: pthread
link: m 
link: ssl 
link: crypto 
link: dl

; 额外头文件路径
inc: ./common
inc: ./common/uthash
inc: ./common/hashtable
inc: ./proxy_server/threadpool
inc: ./proxy_server/ssl
inc: ./proxy_server/sqlite
inc: ./proxy_server/xml
inc: /usr/include

; 额外库文件路径
;lib: /usr/local/opt/jdk/lib

; 临时文件夹
int: objs

; 加入源文件
src: ./common/crc8.c
src: ./common/utility.c
src: ./common/cJSON.c
src: ./common/base64.c
src: ./proxy_server/threadpool/aidsock.c
src: ./proxy_server/threadpool/threadpool.c
src: ./proxy_server/ssl/aidssl.c
src: ./proxy_server/sqlite/sqlite3.c
src: ./proxy_server/sqlite/aid_sql.c
src: ./proxy_server/xml/xml.c
src: ./proxy_server/xml/analysis_soap.c
src: ./proxy_server/proxy_server.c

out: ./bin/proxy_server