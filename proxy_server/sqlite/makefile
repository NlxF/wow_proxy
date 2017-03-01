#!/bin/bash


gcc -g -Wall -c ./sqlite3.c -o ./obj/sqlite3.o

gcc -g -Wall -c ./aid_sql.c -o ./obj/aid_sql.o -I../../common

gcc -g -Wall -c ../../common/hashtable/hashtable.c -o ./obj/hashtable.o -I../../common

gcc -g -o aidsql-test aidsql-test.c ./obj/hashtable.o ./obj/sqlite3.o ./obj/aid_sql.o -I../../common -lpthread -lm -ldl
