/* mylinkedlist - a singularly linked list
 * Copyright (c) 2002 Michael B. Allen <mballen@erols.com>
 *
 * The MIT License
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>


#include "myiterator.h"
#include "mylinkedlist_cache.h"

#define CACHE_SIZE 2

struct node {
	struct node *ptr;
	void *data;
};

struct cache_entry
{
	unsigned int idx;
	struct node * ent;
};

struct mylinkedlist {
	unsigned int max_size;
	unsigned int size;
	struct node *first;
	struct node *last;

	struct cache_entry cache[CACHE_SIZE];
};

static void _cache_clear(struct mylinkedlist * l)
{

	struct cache_entry * ce;
	int i;

	for(i = 0; i < CACHE_SIZE; i++)
	{
		ce = l->cache + i;
		ce->ent = NULL;
		ce->idx = 0;
	}
}
static void _cache_remove_by_node(struct mylinkedlist * l, struct node * n)
{
	struct cache_entry * ce;
	int i;

	for(i = 0; i < CACHE_SIZE; i++)
	{
		ce = l->cache + i;
		if(ce->ent == n)
		{
			ce->ent = NULL;
		}
	}
}


static void _cache_update_by_index(struct mylinkedlist * l, unsigned int idx, int insert)
{
	struct cache_entry * ce;
	int i;
	for(i = 0; i < CACHE_SIZE; i++)
	{
		ce = l->cache + i;
		if(ce->ent && ce->idx >= idx)
		{
			ce->idx += insert? 1 : -1;
		}
	}
}

struct mylinkedlist *
mylinkedlist_new(unsigned int max_size)
{
	struct mylinkedlist *l;

	if ((l = calloc(1, sizeof *l)) == NULL) {
		return NULL;
	}
	memset(l, 0, sizeof(struct mylinkedlist));
	l->max_size = max_size == 0 ? INT_MAX : max_size;
	return l;
}
void
mylinkedlist_del(struct mylinkedlist *l, void (*free_element)(void *))
{
	struct node *next;
	struct node *tmp;

	if (l == NULL) {
		return;
	}
	next = l->first;
	while (next != NULL) {
		if (free_element) {
			free_element(next->data);
		}
		tmp = next;
		next = next->ptr;
		free(tmp);
	}
	free(l);
}

void
mylinkedlist_clear(struct mylinkedlist *l, void (*free_element)(void *))
{
	struct node *next, *tmp;
	int max_size;

	if (l == NULL) {
		return;
	}
	next = l->first;
	while (next != NULL) {
		if (free_element) {
			free_element(next->data);
		}
		tmp = next;
		next = next->ptr;
		free(tmp);
	}
	max_size = l->max_size;
	memset(l, 0, sizeof *l);
	l->max_size = max_size;
}
int
mylinkedlist_add(struct mylinkedlist *l, void *data)
{
	struct node *n;

	if (l == NULL) {
		return -1;
	}
	if (l->size == l->max_size) {
		return -1;
	}
	n = malloc(sizeof *n);
	if (n == NULL) {
		return -1;
	}

	n->data = data;
	n->ptr = NULL;
	if (l->size == 0) {
		l->first = l->last = n;
	} else {
		l->last->ptr = n;
		l->last = n;
	}
	l->size++;

	return 0;
}
int
mylinkedlist_insert(struct mylinkedlist *l, unsigned int idx, void *data)
{
	struct node *n;

	if (l == NULL || data == NULL) {
		return -1;
	}
	if (idx > l->size || l->size == l->max_size) {
		return -1;
	}
	n = malloc(sizeof *n);
	if (n == NULL) {
		return -1;
	}
	n->data = data;
	n->ptr = NULL;
	if (l->size == 0) {
		l->first = l->last = n;
	} else {
		if (idx == 0) {
			n->ptr = l->first;
			l->first = n;
		} else if (idx == l->size) {
			l->last->ptr = n;
			l->last = n;
		} else {
			struct node *tmp, *foo;
			unsigned int i;

			tmp = l->first;
			n->ptr = tmp->ptr;
			for (i = 1; i < idx; i++) {
				tmp = tmp->ptr;
				n->ptr = tmp->ptr;
			}
			foo = n->ptr;
			tmp->ptr = n;
		}
		
	}
	l->size++;

	_cache_update_by_index(l, idx, 1);

	return 0;
}
int
mylinkedlist_insert_sorted(struct mylinkedlist *l,
    int (*compar)(const void *, const void *, void *), void *context, void **replaced, void *data)
{
	struct node *n, *p;
	int cmp;
	unsigned int idx;
	int ins = 1;

	if (l == NULL || compar == NULL || data == NULL) {
		return -1;
	}
	if (l->size == l->max_size) {
		return -1;
	}
	if ((n = malloc(sizeof *n)) == NULL) {
		return -1;
	}
	n->data = data;

	idx = 0;
	p = NULL;
	n->ptr = l->first;
	while (n->ptr) {
		cmp = compar(data, n->ptr->data, context);
		if (cmp < 0 || (replaced && cmp == 0)) {
			if (cmp == 0) {                             /* replace */
				struct node *rm = n->ptr;
				*replaced = rm->data;
				n->ptr = rm->ptr;
				_cache_remove_by_node(l,rm);
				free(rm);
				l->size--;
				ins = 0;
			}
			break;
		}
		p = n->ptr;
		n->ptr = n->ptr->ptr;
		idx++;
	}
	if (p) {
		p->ptr = n;
	} else {
		l->first = n;
	}
	if (n->ptr == NULL) {
		l->last = n;
	}
	l->size++;

	if (ins) {
		_cache_update_by_index(l, idx, 1);
	}

	return idx;
}
int
mylinkedlist_is_empty(const struct mylinkedlist *l)
{
	return l == NULL || l->size == 0;
}
unsigned int
mylinkedlist_size(const struct mylinkedlist *l)
{
	return l == NULL ? 0 : l->size;
}
void
mylinkedlist_iterate(void *this, myiter_t *iter)
{
	struct mylinkedlist *l = this;
	if (l == NULL || iter == NULL) {
		return;
	}
	iter->i1 = 0;
}
void *
mylinkedlist_next(void *this, myiter_t *iter)
{
	struct mylinkedlist *l = this;

	if (l == NULL) {
		return NULL;
	}
	if (iter->i1 >= l->size) {
		return NULL;
	}
	return mylinkedlist_get(l, iter->i1++);
}
void
mylinkedlist_print(struct mylinkedlist *l)
{
	struct node *n = l->first;
	int idx = 0;
	while (n) {
		fprintf(stderr, "list node %d n=%p,data=%p\n", idx, n, n->data);
		n = n->ptr;
		idx++;
	}
}
void *
mylinkedlist_get(struct mylinkedlist *l, unsigned int idx)
{
	if (l == NULL) {
		return NULL;
	}
	if (idx >= l->size) {
		return NULL;
	}
	if (idx == 0) {
		return l->first->data;
	} else if (idx == l->size - 1) {
		return l->last->data;
	} else {
		/*unsigned int i;
		struct node *next = l->first;
	    for(i = 0; i < idx; i++)
		{
			next = next->ptr;
		}
        return next->data;
		*/
		unsigned int i, closest_idx = (unsigned int)-1;
		struct cache_entry *ce, *stale = NULL, *closest = NULL;

		for (i = 0; i < CACHE_SIZE && closest_idx; i++) {
			ce = l->cache + i;
			if (ce->ent == NULL) {
				stale = ce;
				continue;
			}
			if (idx >= ce->idx && (idx - ce->idx) < closest_idx) {
				closest_idx = ce->idx;
				closest = ce;
			} else if (stale == NULL) {
				stale = ce;
			}
		}
		if (closest_idx == (unsigned int)-1) {
			struct node *next = l->first;
			ce = stale;
			for (i = 0; i < idx; i++) {
				next = next->ptr;
			}
			ce->idx = i;
			ce->ent = next;
		} else {
			ce = closest;
			while (ce->idx < idx) {
				ce->idx++;
				ce->ent = ce->ent->ptr;
				if (ce->ent == NULL) {
					return NULL;
				}
			}
		}

		return ce->ent->data;
	}
}
void *
mylinkedlist_get_last(const struct mylinkedlist *l)
{
	if (l == NULL) {
		return NULL;
	}
	if (l->size == 0) {
		return NULL;
	}
	return l->last->data;
}
void *
mylinkedlist_remove(struct mylinkedlist *l, unsigned int idx)
{
	void *result;

	if (l == NULL) {
		return NULL;
	}
	if (idx >= l->size) {
		return NULL;
	}
	if (idx == 0) {
		struct node *tmp;

		result = l->first->data;
		tmp = l->first;
		l->first = l->first->ptr;
		_cache_remove_by_node(l, tmp);
		free(tmp);
	} else {
		struct node *n, *tmp;
		unsigned int i;

		n = l->first;
		for (i = 1; i < idx; i++) {
			n = n->ptr;
		}
		tmp = n->ptr;
		n->ptr = tmp->ptr;
		if (tmp == l->last) {
			l->last = n;
		}

		result = tmp->data;
		_cache_remove_by_node(l, tmp);
		free(tmp);
	}
	
	l->size--;

	_cache_update_by_index(l, idx, 0);

	return result;
}

void *
mylinkedlist_remove_last(struct mylinkedlist *l)
{
	void *result;

	if (l == NULL) {
		return NULL;
	}
	if (l->size == 0) {
		return NULL;
	}
	if (l->size == 1) {
		result = l->first->data;
		_cache_remove_by_node(l, l->first);
		free(l->first);
		l->first = l->last = NULL;
	} else {
		struct node *n;

		result = l->last->data;
		n = l->first;
		while (n->ptr != l->last) {
			n = n->ptr;
		}
		_cache_remove_by_node(l, l->last);
		free(l->last);
		l->last = n;
		n->ptr = NULL;
	}
	l->size--;
	return result;
}
void *
mylinkedlist_remove_data(struct mylinkedlist *l, void *data)
{
	struct node *tmp;

	if (l == NULL) {
		return NULL;
	}
	if (l->size == 0 || data == NULL) {
		return NULL;
	}
	if (data == l->first->data) {
		

		tmp = l->first;
		l->first = l->first->ptr;
		free(tmp);
	} else {
		struct node *n;
		int idx = 1;

		for (n = l->first; n->ptr && n->ptr->data != data; n = n->ptr) {
			idx++;
		}
		if (n->ptr == NULL) {
			return NULL;
		}

		tmp = n->ptr;
		n->ptr = tmp->ptr;
		if (tmp == l->last) {
			l->last = n;
		}
		_cache_update_by_index(l,idx,0);
		free(tmp);
	}
	_cache_remove_by_node(l,tmp);
	l->size--;
	return data;
}
int
mylinkedlist_toarray(struct mylinkedlist *l, void *array[])
{
	struct node *n;
	int i;

	if (l == NULL || array == NULL) {
		return -1;
	}

	n = l->first;
	for (i = 0; n; i++) {
		array[i] = n->data;
		n = n->ptr;
	}

	return 0;
}

void mylinkedlist_remove_if(struct mylinkedlist *l, int (*remove_if)(void * context, const void * obj), void * context)
{
	struct node ** curr;
	struct node * entry;
	int idx = 1;

	_cache_clear(l);

	curr = &l->first;
	while(*curr)
	{
		entry = *curr;
		if(remove_if(context,entry->data))
		{
			//_cache_update_by_index(l,idx,0);
			//_cache_remove_by_node(l,entry);

			if (l->last == entry) 
			{
				l->last = (struct node *)curr;
			}

			*curr = entry->ptr;

			free(entry);
			l->size--;

			
		}
		else
		{
			idx++;
			curr = &entry->ptr;
		}

	}
}

void * mylinkedlist_replace(struct mylinkedlist * l, unsigned int idx, void * data)
{
	void *result;

	if (l == NULL) {
		return NULL;
	}
	if (idx >= l->size) {
		return NULL;
	}
	if (idx == 0) {
		struct node *tmp;

		result = l->first->data;
		l->first->data = data;
	} else {
		struct node *n, *tmp;
		unsigned int i;

		n = l->first;
		for (i = 0; i < idx; i++) {
			n = n->ptr;
		}
		result = n->data;
		n->data = data;
	}
	
	return result;
}