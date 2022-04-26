#ifndef __LIB_KERNEL_PRIORITY_QUEUE_H
#define __LIB_KERNEL_PRIORITY_QUEUE_H

#include "../../threads/thread.h"
/* Functions for max priority queue with struct thread_node */

struct thread_node *tnpq_insert (struct thread_node **root_ptr,
 struct thread_node *node);

struct thread_node *tnpq_delete (struct thread_node **root_ptr,
 struct thread_node *node);

struct thread_node *tnpq_delete_min (struct thread_node **root_ptr);

struct thread_node *tnpq_peek_max (struct thread_node *root);

struct thread_node *tnpq_pop_max (struct thread_node **root_ptr);

struct thread_node *tnpq_update (struct thread_node **root_ptr,
 struct thread_node *node, int64_t effective_priority, int64_t base_priority);


/* Functions for min priority queue with struct sleep_thread */

struct sleep_thread *slpq_insert (struct sleep_thread **root_ptr,
 struct sleep_thread *node);

struct sleep_thread *slpq_delete (struct sleep_thread **root_ptr,
 struct sleep_thread *node);

struct sleep_thread *slpq_delete_min (struct sleep_thread **root_ptr);

struct sleep_thread *slpq_peek_min (struct sleep_thread *root);

struct sleep_thread *slpq_pop_min (struct sleep_thread **root_ptr);

struct sleep_thread *slpq_update (struct sleep_thread **root_ptr,
 struct sleep_thread *node, int64_t new_tick);


#endif