; 指明目标格式：exe, lib, dll 三选一
mode: exe

; 编译选项
flag: -g, -Wall

; 设定链接
;link: stdc++

; 额外头文件路径
inc: ./

; 额外库文件路径
;lib: /usr/local/opt/jdk/lib

; 临时文件夹
int: objs

; 加入源文件
src: crc8.c
src: test_crc8.c

out: test_crc8