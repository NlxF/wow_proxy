#!/bin/bash

gcc -g -Wall -c ../common/crc8.c -o ./obj/crc8.o -I../common

gcc -g -Wall -c ../common/utility.c -o ./obj/utility.o -I../common

gcc -g -Wall -c ../common/cJSON.c -o ./obj/cJSON.o -I../common

gcc -g -Wall -c ./threadpool/aidsock.c -o ./obj/aidsock.o -I./threadpool

gcc -g -Wall -c ./threadpool/threadpool.c -o ./obj/threadpool.o -I./threadpool -I../common

gcc -g -o lserver lserver.c ./obj/crc8.o ./obj/utility.o ./obj/cJSON.o ./obj/aidsock.o ./obj/threadpool.o  -I../common -I./thraedpool -lpthread -lm

rm ./obj/*


