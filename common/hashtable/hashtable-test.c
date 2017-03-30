/*
  From https://www.cs.cmu.edu/~fp/courses/15122-f10/lectures/22-mem/hashtable-test.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "hashtable.h"
#include "contracts.h"

void test()
{
    int n = (1<<10)+1; // start with 1<<10 for timing; 1<<9 for -d
    int num_tests = 1; // start with 1000 for timing; 10 for -d
    int i; int j;
    
    /* different from C0! */
    printf("Testing array of size %d with %d values, %d times\n", n/5, n, num_tests);
    for (j = 0; j < num_tests; j++)
    {
        table H = table_new( n/5 );
        for (i = 0; i < n; i++)
        {
            elem e = malloc(sizeof(struct elem));
            e->word = make_key_by_int(j*n+i);	/* diff from C0 */
            printf("%s%s%s%d\n", "make key:", e->word," for value:", j*n+i);
            e->info = j*n+i;
            table_insert(H, e);
        }
        for (i = 0; i < n; i++)
        {
            char* s = make_key_by_int(j*n+i);
            assert(((elem)table_search(H, s))->info == j*n+i); /* "missed existing element" */
            free(s);
        }
        for (i = 0; i < n; i++)
        {
            char* s = make_key_by_int((j+1)*n+i);
            assert(table_search(H, s) == NULL); /* "found nonexistent element" */
            free(s);
        }
        table_free(H);
    }
    printf("All tests passed!\n");
}


int main ()
{
    test();
    
    return 0;
}
