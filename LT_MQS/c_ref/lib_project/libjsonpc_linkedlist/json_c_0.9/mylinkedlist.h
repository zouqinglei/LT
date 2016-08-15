#ifndef MY_LINKEDLIST_H
#define MY_LINKEDLIST_H

/* mylinkedlist - a singularly linked list
 */

#ifdef __cplusplus
extern "C" {
#endif


#include "myiterator.h"

struct mylinkedlist;

struct mylinkedlist *mylinkedlist_new(unsigned int);
void mylinkedlist_del(struct mylinkedlist *l, void (*free_data_fn)(void *));

void mylinkedlist_clear(struct mylinkedlist *l, void (*free_data_fn)(void *));
int mylinkedlist_add(struct mylinkedlist *l, void *data);
int mylinkedlist_insert(struct mylinkedlist *l, unsigned int idx, void *data);
int mylinkedlist_insert_sorted(struct mylinkedlist *l,
    int (*compar)(const void *, const void *), void **replaced, void *data);
int mylinkedlist_is_empty(const struct mylinkedlist *l);
unsigned int mylinkedlist_size(const struct mylinkedlist *l);
void *mylinkedlist_get(struct mylinkedlist *l, unsigned int idx);
void *mylinkedlist_get_last(const struct mylinkedlist *l);
void mylinkedlist_iterate(void *l, myiter_t *iter);
void *mylinkedlist_next(void *l, myiter_t *iter);
void *mylinkedlist_remove(struct mylinkedlist *l, unsigned int idx);
void *mylinkedlist_remove_last(struct mylinkedlist *l);
void *mylinkedlist_remove_data(struct mylinkedlist *l, void *data);
int mylinkedlist_toarray(struct mylinkedlist *l, void *array[]);

void mylinkedlist_remove_if(struct mylinkedlist *l, int (*remove_if)(void * context, const void * obj), void * context);
void * mylinkedlist_replace(struct mylinkedlist * l, unsigned int idx, void * data);


#ifdef __cplusplus
}
#endif

#endif /* MY_LINKEDLIST_H */

