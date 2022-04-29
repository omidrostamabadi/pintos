#ifndef __LIB_KERNEL_PRIORITY_QUEUE_H
#define __LIB_KERNEL_PRIORITY_QUEUE_H


/* Functions for max priority queue with struct thread_node */

// struct thread_node *tnpq_insert (struct thread_node **root_ptr,
//  struct thread_node *node);

// struct thread_node *tnpq_delete (struct thread_node **root_ptr,
//  struct thread_node *node);

// struct thread_node *tnpq_delete_min (struct thread_node **root_ptr);

// struct thread_node *tnpq_peek_max (struct thread_node *root);

// struct thread_node *tnpq_pop_max (struct thread_node **root_ptr);

// struct thread_node *tnpq_update (struct thread_node **root_ptr,
//  struct thread_node *node, int64_t effective_priority, int64_t base_priority);

struct thread *tnpq_insert (struct thread **root_ptr,
 struct thread *node);

struct thread *tnpq_delete (struct thread **root_ptr,
 struct thread *node);

struct thread *tnpq_search (struct thread *root,
struct thread *node);

struct thread *tnpq_peek_max (struct thread *root);

struct thread *tnpq_pop_max (struct thread **root_ptr);



struct thread *snpq_insert (struct thread **root_ptr,
 struct thread *node);

struct thread *snpq_delete (struct thread **root_ptr,
 struct thread *node);

struct thread *snpq_search (struct thread *root,
struct thread *node);

struct thread *snpq_peek_max (struct thread *root);

struct thread *snpq_pop_max (struct thread **root_ptr);


/* Functions for min priority queue with struct thread_sleep */

struct thread_sleep *slpq_insert (struct thread_sleep **root_ptr,
 struct thread_sleep *node);

struct thread_sleep *slpq_delete (struct thread_sleep **root_ptr,
 struct thread_sleep *node);

struct thread_sleep *slpq_delete_min (struct thread_sleep **root_ptr);

struct thread_sleep *slpq_peek_min (struct thread_sleep *root);

struct thread_sleep *slpq_pop_min (struct thread_sleep **root_ptr);

struct thread_sleep *slpq_update (struct thread_sleep **root_ptr,
 struct thread_sleep *node, int64_t new_tick);
#endif