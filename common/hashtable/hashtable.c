#include <stdbool.h>
#include <stdlib.h>
#include "hashtable.h"

void* xcalloc(size_t nobj, size_t size)
{
    void* p = calloc(nobj, size);
    if (p == NULL)
    {
        printf("allocation failed\n");
        abort();
    }
    return p;
}

void* xmalloc(size_t size)
{
    void* p = malloc(size);
    if (p == NULL)
    {
        printf("allocation failed\n");
        abort();
    }
    memset(p, 0, size);
    return p;
}

/* Interface type definitions */
/* see hashtable.h */

/* Chains, implemented as linked lists */
typedef struct chain* chain;

/* alpha = n/m = num_elems/size */
struct table
{
    int size;			/* m */
    int num_elems;		/* n */
    chain* array;			/* \length(array) == size */
    ht_key (*elem_key)(ht_elem e); /* extracting keys from elements */
    bool (*equal)(ht_key k1, ht_key k2); /* comparing keys */
    int (*hash)(ht_key k, int m);	       /* hashing keys */
};

struct list
{
    ht_elem data;
    struct list* next;
};
typedef struct list* list;
/* linked lists may be NULL (= end of list) */
/* we do not check for circularity */

void list_free(list p, void (*elem_free)(ht_elem e))
{
    list q;
    while (p != NULL)
    {
        if (p->data != NULL && elem_free != NULL)
        /* free element, if such a function is supplied */
            (*elem_free)(p->data);
        q = p->next;
        free(p);
        p = q;
    }
}

/* chains */

chain chain_new ();
ht_elem chain_insert(table H, chain C, ht_elem e);
ht_elem chain_search(table H, chain C, ht_key k);
void chain_free(chain C, void (*elem_free)(ht_elem e));

struct chain
{
    list list;
};

/* valid chains are not null */
bool is_chain(chain C)
{
    return C != NULL;
}

chain chain_new()
{
    chain C = xmalloc(sizeof(struct chain));
    C->list = NULL;
    ENSURES(is_chain(C));
    return C;
}

/* key comparison */
bool equal(ht_key s1, ht_key s2)
{
    return strcmp((char*)s1,(char*)s2) == 0;		/* different from C0! or: !strcmp(s1,s2); */
}

/* extracting keys from elements */
ht_key elem_key(ht_elem e)
{
    REQUIRES(e != NULL);
    return ((elem)e)->word;
}

/* make_key function */
char* make_key(int n)
{
    char* buf = malloc(MAXINT_CHARS * sizeof(char));
    snprintf(buf, MAXINT_CHARS, "%d", n);
    return buf;
}

/* hash function */
/* uses pseudo-random number generation */
/* converted to use unsigned int in C */
int hash(ht_key s, int m)
{
    REQUIRES(m > 1);
    unsigned int a = 1664525;
    unsigned int b = 1013904223;	/* inlined random number generator */
    unsigned int r = 0xdeadbeef;	       /* initial seed */
    int len = strlen(s);		       /* different from C0! */
    int i; unsigned int h = 0;	       /* empty string maps to 0 */
    for (i = 0; i < len; i++)
    {
        h = r*h + ((char*)s)[i];	 /* mod 2^32 */
        r = r*a + b;	 /* mod 2^32, linear congruential random no */
    }
    h = h % m;			/* reduce to range */
    //@assert -m < (int)h && (int)h < m;
    int hx = (int)h;
    if (hx < 0) h += m;	/* make positive, if necessary */
    ENSURES(0 <= hx && hx < m);
    
//    printf("key=%2s hash value=%d, index=%d with chain size=%d\n", s, hx, hx, m);
    return hx;
}


/* free elem function */
void _elem_free(ht_elem e)
{
    free(((elem)e)->word);
    free(e);
}

/* chain_find(p, k) returns list element whose
 * data field has key k, or NULL if none exists
 */
list chain_find(table H, chain C, ht_key k)
{
    REQUIRES(is_chain(C));
    list p = C->list;
    while (p != NULL)
    {
        if ((*H->equal)(k, (*H->elem_key)(p->data)))
            return p;
        p = p->next;
    }
    return NULL;
}

ht_elem chain_insert(table H, chain C, ht_elem e)
{
    REQUIRES(is_chain(C) && e != NULL);
    list p = chain_find(H, C, (*H->elem_key)(e));
    if (p == NULL)
    {
        /* insert new element at the beginning */
        list new_item = xmalloc(sizeof(struct list));
        new_item->data = e;
        new_item->next = C->list;
        C->list = new_item;
        ENSURES(is_chain(C));
        return NULL;		/* did not overwrite entry */
    } else
    {
        /* overwrite existing entry with given key */
        ht_elem old_e = p->data;
        p->data = e;
        ENSURES(is_chain(C));
        return old_e;		/* return old entry */
    }
}

ht_elem chain_search(table H, chain C, ht_key k)
{
    REQUIRES(is_chain(C));
    list p = chain_find(H, C, k);
    if (p == NULL) return NULL;
    else return p->data;
}


void chain_free(chain C, void (*elem_free)(ht_elem e))
{
    REQUIRES(is_chain(C));
    list_free(C->list, elem_free);
    free(C);
}

/* Hash table interface */
/* see hashtable.h */

/* Hash table implementation */

/* is_h_chain(C, h, m) - all of chain C's keys are equal to h */
/* keys should also be pairwise distinct, but we do not check that */
/* table size is m */
bool is_h_chain (table H, chain C, int h, int m)
{
    REQUIRES(0 <= h && h < m);
    if (C == NULL) return false;
    list p = C->list;
    while (p != NULL)
    {
        if (p->data == NULL) return false;
        if ((*H->hash)((*H->elem_key)(p->data),m) != h)
            return false;
        p = p->next;
    }
    return true;
}

bool is_table(table H)
//@requires H != NULL && H->size == \length(H->array);
{
    int i; int m;
    /* array elements may be NULL or chains */
    if (H == NULL) return false;
    m = H->size;
    for (i = 0; i < m; i++)
    {
        chain C = H->array[i];
        if (!(C == NULL || is_h_chain(H, C, i, m))) return false;
    }
    return true;
}

table table_new(int init_size)
{
    REQUIRES(init_size > 1);
    chain* A = xcalloc(init_size, sizeof(chain));
    table H = xmalloc(sizeof(struct table));
    H->size = init_size;
    H->num_elems = 0;
    H->array = A;			/* all initialized to NULL; */
    H->elem_key = elem_key;
    H->equal = equal;
    H->hash = hash;
    ENSURES(is_table(H));
    return H;
}

ht_elem table_insert(table H, ht_elem e)
{
    REQUIRES(is_table(H));
    ht_elem old_e;
    ht_key k = (*H->elem_key)(e);
    int h = (*H->hash)(k, H->size);
    if (H->array[h] == NULL)
        H->array[h] = chain_new();
    old_e = chain_insert(H, H->array[h], e);
    if (old_e != NULL) return old_e;
    H->num_elems++;
    ENSURES(is_table(H));
    ENSURES(table_search(H, (*H->elem_key)(e)) == e); /* pointer equality */
    return NULL;
}

ht_elem table_search(table H, ht_key k)
{
    REQUIRES(is_table(H));
    int h = (*H->hash)(k, H->size);
    if (H->array[h] == NULL) return NULL;
    ht_elem e = chain_search(H, H->array[h], k);
    ENSURES(e == NULL || (*H->equal)((*H->elem_key)(e), k));
    return e;
}

void table_free(table H)
{
    REQUIRES(is_table(H));
    void (*elem_free)(ht_elem e);
    elem_free = _elem_free;
    int i;
    for (i = 0; i < H->size; i++)
    {
        chain C = H->array[i];
        if (C != NULL) chain_free(C, elem_free);
    }
    free(H->array);
    free(H);
}
