/*
    zouqinglei@163.com 
    All right reserved.
*/

#ifndef MY_ITERATOR_H
#define MY_ITERATOR_H

/* iter - container for iterator state
 */

typedef struct _myiter {
	unsigned long i1;
	unsigned long i2;
	unsigned long i3;
	void *p;
} myiter_t;

typedef void (*myiterate_fn)(void *obj, myiter_t *iter);
typedef void *(*myiterate_next_fn)(void *obj, myiter_t *iter);

#endif /* MY_ITERATOR_H */
