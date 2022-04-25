#ifndef __LIB_KERNEL_PRIORITY_QUEUE_H
#define __LIB_KERNEL_PRIORITY_QUEUE_H

struct thread_node *tnpq_insert (struct thread_node *root,
 struct thread_node *node);

struct thread_node *tnpq_search (struct thread_node *root,
 struct thread_node *node);


#endif