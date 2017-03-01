#!/bin/bash

gcc -g -Wall -c ../common/crc8.c -o ./obj/crc8.o -I../common

gcc -g -Wall -c ../common/utility.c -o ./obj/utility.o -I../common

gcc -g -Wall -c ../common/cJSON.c -o ./obj/cJSON.o -I../common

gcc -g -Wall -c ../common/hashtable/hashtable.c -o ./obj/hashtable.o -I../common

gcc -g -Wall -c ./threadpool/aidsock.c -o ./obj/aidsock.o -I./threadpool

gcc -g -Wall -c ./threadpool/threadpool.c -o ./obj/threadpool.o -I./threadpool -I../common

gcc -g -Wall -c ./ssl/aidssl.c -o ./obj/aidssl.o -I/usr/include

gcc -g -Wall -c ./sqlite/sqlite3.c -o ./obj/sqlite3.o -I./sqlite

gcc -g -Wall -c ./sqlite/aid_sql.c -o ./obj/aid_sql.o -I../common

gcc -g -o proxy_server proxy_server.c ./obj/crc8.o ./obj/utility.o ./obj/cJSON.o ./obj/aidsock.o ./obj/aidssl.o ./obj/threadpool.o ./obj/hashtable.o ./obj/sqlite3.o ./obj/aid_sql.o -I../common -I./thraedpool -lpthread -lm -lssl -lcrypto

rm ./obj/*


