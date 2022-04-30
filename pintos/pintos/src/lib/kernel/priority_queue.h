#ifndef __LIB_KERNEL_PRIORITY_QUEUE_H
#define __LIB_KERNEL_PRIORITY_QUEUE_H


/* Functions for max priority queue with struct thread */

struct thread *tnpq_insert (struct thread **root_ptr,
 struct thread *node);

struct thread *tnpq_delete (struct thread **root_ptr,
 struct thread *node);

struct thread *tnpq_search (struct thread *root,
struct thread *node);

struct thread *tnpq_peek_max (struct thread *root);

struct thread *tnpq_pop_max (struct thread **root_ptr);


/* Functions for max priority queue with struct thread */
struct thread *snpq_insert (struct thread **root_ptr,
 struct thread *node);

struct thread *snpq_delete (struct thread **root_ptr,
 struct thread *node);

struct thread *snpq_search (struct thread *root,
struct thread *node);

struct thread *snpq_peek_max (struct thread *root);

struct thread *snpq_pop_max (struct thread **root_ptr);



#endif