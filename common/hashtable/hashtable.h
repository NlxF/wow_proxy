#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include "contracts.h"

typedef void* ht_key;
typedef void* ht_elem;

/* max number of character in int: 10 + sign + '\0' = 12 */
#define MAXINT_CHARS 12

/* elements */
struct elem
{
    char* word;			  /* key */
    void* info;           /* value */
};

typedef struct elem* elem;


/* Hash table interface */
char* make_key_by_int(int n);

typedef struct table* table;

table table_new (int init_size);

ht_elem table_insert(table H, ht_elem e);

ht_elem table_search(table H, ht_key k);

void table_free(table H);

#endif
