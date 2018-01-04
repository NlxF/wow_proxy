; 指明目标格式：exe, lib, dll 三选一
mode: exe

; 编译选项
flag: -Wall, -O3, -g3

; 使用库
;link: m, libstdc++.a 
;link: c++0x
;link: stdc++

; 添加额外的 include 目录 和 lib 目录的话：
;inc: gsoap/
;lib: /usr/local/opt/jdk/lib

; 加入源文件
src: soapC.c
src: soapServer.c
src: stdsoap2.c
src: gsserver.c

; 指定临时文件夹
int: objs

; 指定输出文件名
out: gsserver