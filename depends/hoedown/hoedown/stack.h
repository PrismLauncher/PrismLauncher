/* stack.h - simple stacking */

#ifndef HOEDOWN_STACK_H
#define HOEDOWN_STACK_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/*********
 * TYPES *
 *********/

struct hoedown_stack {
	void **item;
	size_t size;
	size_t asize;
};
typedef struct hoedown_stack hoedown_stack;


/*************
 * FUNCTIONS *
 *************/

/* hoedown_stack_init: initialize a stack */
void hoedown_stack_init(hoedown_stack *st, size_t initial_size);

/* hoedown_stack_uninit: free internal data of the stack */
void hoedown_stack_uninit(hoedown_stack *st);

/* hoedown_stack_grow: increase the allocated size to the given value */
void hoedown_stack_grow(hoedown_stack *st, size_t neosz);

/* hoedown_stack_push: push an item to the top of the stack */
void hoedown_stack_push(hoedown_stack *st, void *item);

/* hoedown_stack_pop: retrieve and remove the item at the top of the stack */
void *hoedown_stack_pop(hoedown_stack *st);

/* hoedown_stack_top: retrieve the item at the top of the stack */
void *hoedown_stack_top(const hoedown_stack *st);


#ifdef __cplusplus
}
#endif

#endif /** HOEDOWN_STACK_H **/
