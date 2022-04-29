#include <stdio.h>
#include "threads/thread.h"
#include "threads/synch.h"
#include "priority_queue.h"

/* Functions for max priority queue with struct thread_node */
struct thread *
tnpq_insert (struct thread **root_ptr,
 struct thread *node)
{
  struct thread *root = *root_ptr;
  if(root == NULL) // Tree is empty
    {
        *root_ptr = node;
        root = *root_ptr;
        root->left_child = root->right_child = NULL;
    }
  else
    {
      struct thread *parent = root;
      struct thread *tmp = root;
      /* Find the place of node in the tree */
      while (tmp != NULL)
        {
          parent = tmp;
          if (node->effective_priority >= tmp->effective_priority)
            { // Go to right subtree
              tmp = tmp->right_child;
            }
          else
            {
              tmp = tmp->left_child;
            }
        }
      /* Insert and set the pointers */
      if (node->effective_priority >= parent->effective_priority)
        {
          parent->right_child = node;
        }
      else
        {
          parent->left_child = node;
        }
    }
    
    return root;
}

struct thread *tnpq_search (struct thread *root,
 struct thread *node)
{
  if (root == NULL)
    return NULL;
  
  struct thread *parent;
  struct thread *tmp = root;
  /* Binary search */
  while (tmp != NULL)
    {
      if (node->effective_priority < tmp->effective_priority)
        {
          tmp = tmp->left_child;
        }
      else if (node->effective_priority > tmp->effective_priority)
        {
          tmp = tmp->right_child;
        }
      else
        {
          if (node == tmp)
            {
              return tmp;
            }
          /* If we reach here, there must be another duplicate with
              the same priority, so we go to the right subtree to find that */
            tmp = tmp->right_child;
        }
    }
  return NULL; // Could not find the node
}

struct thread *tnpq_delete (struct thread **root_ptr,
 struct thread *node)
{
  struct thread *root = *root_ptr;

  /* Special case of empty tree */
  if (root == NULL)
    return NULL;
  
  /* Special case of deleting root */
  if (node == root)
    {
      if (root->right_child != NULL && root->left_child != NULL)
        { // With two children
          struct thread *successor = tnpq_delete_min (&root->right_child);
          *root_ptr = successor;
          return root;
        }
      else if (root->left_child != NULL)
        { // One child
          *root_ptr = root->left_child;
          return root;
        }
      else
        { // One or no child
          *root_ptr = root->right_child;
          return root;
        }
    }
  
  struct thread *parent;
  struct thread *tmp = root;

  /* Binary search */
  while (tmp != NULL)
    {
      // parent = tmp;
      if (node->effective_priority < tmp->effective_priority)
        {
          parent = tmp;
          tmp = tmp->left_child;
        }
      else if (node->effective_priority > tmp->effective_priority)
        {
          parent = tmp;
          tmp = tmp->right_child;
        }
      else
        {
          if (node == tmp)
            {
              if (tmp->right_child == NULL && tmp->left_child == NULL)
                { // Node with no children
                  if (tmp->effective_priority < parent->effective_priority)
                    { // Is left child of its parent
                      parent->left_child = NULL;
                      return tmp;
                    }
                  else
                    {
                      parent->right_child = NULL;
                      return tmp;
                    }
                }
              else if (tmp->right_child != NULL && tmp->left_child != NULL)
                { // Node with two children
                  struct thread *successor = tnpq_delete_min (&tmp->right_child);
                  successor->left_child = tmp->left_child;
                  successor->right_child = tmp->right_child;
                  if (tmp->effective_priority < parent->effective_priority)
                    { // Is left child of its parent
                      parent->left_child = successor;
                      return tmp;
                    }
                  else
                    {
                      parent->right_child = successor;
                      return tmp;
                    }
                }
              else if (tmp->right_child != NULL)
                { // Node with one child
                  if (tmp->effective_priority < parent->effective_priority)
                    {
                      parent->left_child = tmp->right_child;
                      return tmp;
                    }
                  else
                    {
                      parent->right_child = tmp->right_child;
                      return tmp;
                    }
                }
              else
                {
                  if (tmp->effective_priority < parent->effective_priority)
                    {
                      parent->left_child = tmp->left_child;
                      return tmp;
                    }
                  else
                    {
                      parent->right_child = tmp->left_child;
                      return tmp;
                    }
                }
            }
          /* If we reach here, there must be another duplicate with
              the same priority, so we go to the right subtree to find that */
            parent = tmp;
            tmp = tmp->right_child;
        }
    }
  return NULL; // Could not find the node
}

struct thread *tnpq_delete_min (struct thread **root_ptr)
{
  struct thread *root = *root_ptr;
  if (root == NULL)
    return NULL;
  struct thread *parent = root;
  struct thread *tmp = root;
  while (tmp->left_child != NULL)
    {
      parent = tmp;
      tmp = tmp->left_child;
    }
  if (tmp == root)
    {
      *root_ptr = tmp->right_child;
    }
  else
    {
      parent->left_child = tmp->right_child;
    }
  return tmp;
}

struct thread *tnpq_peek_max (struct thread *root)
{
  if (root == NULL)
    return NULL;
  struct thread *tmp = root;
  while (tmp->right_child != NULL)
    tmp = tmp->right_child;
  return tmp;
}

struct thread *tnpq_pop_max (struct thread **root_ptr)
{
  struct thread *root = *root_ptr;
  if (root == NULL)
    return NULL;
  struct thread *tmp = root;
  while (tmp->right_child != NULL)
    {
      tmp = tmp->right_child;
    }
  tnpq_delete (root_ptr, tmp);
  if (tmp != NULL)
    {
      tmp->left_child = NULL;
      tmp->right_child = NULL;
    }
  return tmp;
}

struct thread *tnpq_update (struct thread **root_ptr,
 struct thread *node, int64_t effective_priority, int64_t base_priority)
{
  if (tnpq_delete (root_ptr, node))
    {
      node->effective_priority = effective_priority;
      node->base_priority = base_priority;
      // Update priority in struct thread as well
      struct thread *new_node = tnpq_insert (root_ptr, node);
      return new_node;
    }
  return NULL;
}

/* Functions for min priority queue with struct thread_sleep */

struct thread_sleep *
slpq_insert (struct thread_sleep **root_ptr,
 struct thread_sleep *node)
{
  struct thread_sleep *root = *root_ptr;
  if(root == NULL) // Tree is empty
    {
        *root_ptr = node;
        root = *root_ptr;
        root->left_child = root->right_child = NULL;
    }
  else
    {
      struct thread_sleep *parent = root;
      struct thread_sleep *tmp = root;
      /* Find the place of node in the tree */
      while (tmp != NULL)
        {
          parent = tmp;
          if (node->final_tick >= tmp->final_tick)
            { // Go to right subtree
              tmp = tmp->right_child;
            }
          else
            {
              tmp = tmp->left_child;
            }
        }
      /* Insert and set the pointers */
      if (node->final_tick >= parent->final_tick)
        {
          parent->right_child = node;
        }
      else
        {
          parent->left_child = node;
        }
    }
    
    return root;
}

struct thread_sleep *slpq_delete (struct thread_sleep **root_ptr,
 struct thread_sleep *node)
{
  struct thread_sleep *root = *root_ptr;

  /* Special case of empty tree */
  if (root == NULL)
    return NULL;
  
  /* Special case of deleting root */
  if (node == root)
    {
      if (root->right_child != NULL)
        {
          struct thread_sleep *successor = slpq_delete_min (&root->right_child);
          *root_ptr = successor;
          return root;
        }
      else
        {
          *root_ptr = root->left_child;
          return root;
        }
    }
  
  struct thread_sleep *parent;
  struct thread_sleep *tmp = root;

  /* Binary search */
  while (tmp != NULL)
    {
      // parent = tmp;
      if (node->final_tick < tmp->final_tick)
        {
          parent = tmp;
          tmp = tmp->left_child;
        }
      else if (node->final_tick > tmp->final_tick)
        {
          parent = tmp;
          tmp = tmp->right_child;
        }
      else
        {
          if (node->its_thread == tmp->its_thread)
            {
              if (tmp->right_child == NULL && tmp->left_child == NULL)
                { // Node with no children
                  if (tmp->final_tick < parent->final_tick)
                    { // Is left child of its parent
                      parent->left_child = NULL;
                      return tmp;
                    }
                  else
                    {
                      parent->right_child = NULL;
                      return tmp;
                    }
                }
              else if (tmp->right_child != NULL || tmp->left_child != NULL)
                { // Node with two children
                  struct thread_sleep *successor = slpq_delete_min (&tmp->right_child);
                  successor->left_child = tmp->left_child;
                  successor->right_child = tmp->right_child;
                  if (tmp->final_tick < parent->final_tick)
                    { // Is left child of its parent
                      parent->left_child = successor;
                      return tmp;
                    }
                  else
                    {
                      parent->right_child = successor;
                      return tmp;
                    }
                }
            }
          /* If we reach here, there must be another duplicate with
              the same priority, so we go to the right subtree to find that */
            parent = tmp;
            tmp = tmp->right_child;
        }
    }
  return NULL; // Could not find the node
}

struct thread_sleep *slpq_delete_min (struct thread_sleep **root_ptr)
{
  struct thread_sleep *root = *root_ptr;
  if (root == NULL)
    return NULL;
  struct thread_sleep *parent = root;
  struct thread_sleep *tmp = root;
  while (tmp->left_child != NULL)
    {
      parent = tmp;
      tmp = tmp->left_child;
    }
  if (tmp == root)
    {
      *root_ptr = tmp->right_child;
    }
  else
    {
      parent->left_child = tmp->right_child;
    }
  return tmp;
}

struct thread_sleep *slpq_peek_min (struct thread_sleep *root)
{
  if (root == NULL)
    return NULL;
  struct thread_sleep *tmp = root;
  while (tmp->left_child != NULL)
    tmp = tmp->left_child;
  return tmp;
}

struct thread_sleep *slpq_pop_min (struct thread_sleep **root_ptr)
{
  struct thread_sleep *root = *root_ptr;
  if (root == NULL)
    return NULL;
  struct thread_sleep *tmp = root;
  while (tmp->left_child != NULL)
    {
      tmp = tmp->left_child;
    }
  slpq_delete (root_ptr, tmp);
  return tmp;
}

struct thread_sleep *slpq_update (struct thread_sleep **root_ptr,
 struct thread_sleep *node, int64_t new_tick)
{
  slpq_delete(root_ptr, node);
  node->final_tick = new_tick;
  struct thread_sleep *new_node = slpq_insert(root_ptr, node);
  return new_node;
}



/* Functions for max priority queue with struct thread_node */
struct semaphore_elem *
swpq_insert (struct semaphore_elem **root_ptr,
             struct semaphore_elem *node)
{
    struct semaphore_elem *root = *root_ptr;
    if(root == NULL) // Tree is empty
    {
        *root_ptr = node;
        root = *root_ptr;
        root->left_child = root->right_child = NULL;
    }
    else
    {
        struct semaphore_elem *parent = root;
        struct semaphore_elem *tmp = root;
        /* Find the place of node in the tree */
        while (tmp != NULL)
        {
            parent = tmp;
            if (node->effective_priority >= tmp->effective_priority)
            { // Go to right subtree
                tmp = tmp->right_child;
            }
            else
            {
                tmp = tmp->left_child;
            }
        }
        /* Insert and set the pointers */
        if (node->effective_priority >= parent->effective_priority)
        {
            parent->right_child = node;
        }
        else
        {
            parent->left_child = node;
        }
    }

    return root;
}

struct semaphore_elem *swpq_pop_max (struct semaphore_elem **root_ptr)
{
    struct semaphore_elem *root = *root_ptr;
    if (root == NULL)
        return NULL;
    struct semaphore_elem *tmp = root;
    while (tmp->right_child != NULL)
    {
        tmp = tmp->right_child;
    }
    swpq_delete (root_ptr, tmp);
    return tmp;
}

struct semaphore_elem *swpq_delete (struct semaphore_elem **root_ptr,
                                 struct semaphore_elem *node)
{
    struct semaphore_elem *root = *root_ptr;

    /* Special case of empty tree */
    if (root == NULL)
        return NULL;

    /* Special case of deleting root */
    if (node == root)
    {
        if (root->right_child != NULL)
        {
            struct semaphore_elem *successor = swpq_delete_min (&root->right_child);
            *root_ptr = successor;
            return root;
        }
        else
        {
            *root_ptr = root->left_child;
            return root;
        }
    }

    struct semaphore_elem *parent;
    struct semaphore_elem *tmp = root;

    /* Binary search */
    while (tmp != NULL)
    {
        // parent = tmp;
        if (node->effective_priority < tmp->effective_priority)
        {
            parent = tmp;
            tmp = tmp->left_child;
        }
        else if (node->effective_priority > tmp->effective_priority)
        {
            parent = tmp;
            tmp = tmp->right_child;
        }
        else
        {

            if (tmp->right_child == NULL && tmp->left_child == NULL)
            { // Node with no children
                if (tmp->effective_priority < parent->effective_priority)
                { // Is left child of its parent
                    parent->left_child = NULL;
                    return tmp;
                }
                else
                {
                    parent->right_child = NULL;
                    return tmp;
                }
            }
            else if (tmp->right_child != NULL || tmp->left_child != NULL)
            { // Node with two children
                struct semaphore_elem *successor = swpq_delete_min (&tmp->right_child);
                successor->left_child = tmp->left_child;
                successor->right_child = tmp->right_child;
                if (tmp->effective_priority < parent->effective_priority)
                { // Is left child of its parent
                    parent->left_child = successor;
                    return tmp;
                }
                else
                {
                    parent->right_child = successor;
                    return tmp;
                }
            }


        }
    }
    return NULL; // Could not find the node
}

struct semaphore_elem *swpq_delete_min (struct semaphore_elem **root_ptr)
{
    struct semaphore_elem *root = *root_ptr;
    if (root == NULL)
        return NULL;
    struct semaphore_elem *parent = root;
    struct semaphore_elem *tmp = root;
    while (tmp->left_child != NULL)
    {
        parent = tmp;
        tmp = tmp->left_child;
    }
    if (tmp == root)
    {
        *root_ptr = tmp->right_child;
    }
    else
    {
        parent->left_child = tmp->right_child;
    }
    return tmp;
}