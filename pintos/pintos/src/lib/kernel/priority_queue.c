#include <stdio.h>
#include "priority_queue.h"

/* Functions for max priority queue with struct thread_node */
struct thread_node *
tnpq_insert (struct thread_node **root_ptr,
 struct thread_node *node)
{
  struct thread_node *root = *root_ptr;
  if(root == NULL) // Tree is empty
    {
        *root_ptr = node;
        root = *root_ptr;
        root->left_child = root->right_child = NULL;
    }
  else
    {
      struct thread_node *parent = root;
      struct thread_node *tmp = root;
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

struct thread_node *tnpq_search (struct thread_node *root,
 struct thread_node *node)
{
  if (root == NULL)
    return NULL;
  
  struct thread_node *parent;
  struct thread_node *tmp = root;
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
          if (node->its_thread == tmp->its_thread)
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

struct thread_node *tnpq_delete (struct thread_node **root_ptr,
 struct thread_node *node)
{
  struct thread_node *root = *root_ptr;

  /* Special case of empty tree */
  if (root == NULL)
    return NULL;
  
  /* Special case of deleting root */
  if (node == root)
    {
      if (root->right_child != NULL)
        {
          struct thread_node *successor = tnpq_delete_min (&root->right_child);
          *root_ptr = successor;
          return root;
        }
      else
        {
          *root_ptr = root->left_child;
          return root;
        }
    }
  
  struct thread_node *parent;
  struct thread_node *tmp = root;

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
          if (node->its_thread == tmp->its_thread)
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
                  struct thread_node *successor = tnpq_delete_min (&tmp->right_child);
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
          /* If we reach here, there must be another duplicate with
              the same priority, so we go to the right subtree to find that */
            parent = tmp;
            tmp = tmp->right_child;
        }
    }
  return NULL; // Could not find the node
}

struct thread_node *tnpq_delete_min (struct thread_node **root_ptr)
{
  struct thread_node *root = *root_ptr;
  if (root == NULL)
    return NULL;
  struct thread_node *parent = root;
  struct thread_node *tmp = root;
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

struct thread_node *tnpq_peek_max (struct thread_node *root)
{
  if (root == NULL)
    return NULL;
  struct thread_node *tmp = root;
  while (tmp->right_child != NULL)
    tmp = tmp->right_child;
  return tmp;
}

struct thread_node *tnpq_pop_max (struct thread_node **root_ptr)
{
  struct thread_node *root = *root_ptr;
  if (root == NULL)
    return NULL;
  struct thread_node *tmp = root;
  while (tmp->right_child != NULL)
    {
      tmp = tmp->right_child;
    }
  tnpq_delete (root_ptr, tmp);
  return tmp;
}

struct thread_node *tnpq_update (struct thread_node **root_ptr,
 struct thread_node *node, int64_t effective_priority, int64_t base_priority)
{
  tnpq_delete (root_ptr, node);
  node->effective_priority = effective_priority;
  node->base_priority = base_priority;
  // Update priority in struct thread as well
  struct thread_node *new_node = tnpq_insert (root_ptr, node);
  return new_node;
}

/* Functions for min priority queue with struct sleep_thread */

struct sleep_thread *
slpq_insert (struct sleep_thread **root_ptr,
 struct sleep_thread *node)
{
  struct sleep_thread *root = *root_ptr;
  if(root == NULL) // Tree is empty
    {
        *root_ptr = node;
        root = *root_ptr;
        root->left_child = root->right_child = NULL;
    }
  else
    {
      struct sleep_thread *parent = root;
      struct sleep_thread *tmp = root;
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

struct sleep_thread *slpq_delete (struct sleep_thread **root_ptr,
 struct sleep_thread *node)
{
  struct sleep_thread *root = *root_ptr;

  /* Special case of empty tree */
  if (root == NULL)
    return NULL;
  
  /* Special case of deleting root */
  if (node == root)
    {
      if (root->right_child != NULL)
        {
          struct sleep_thread *successor = slpq_delete_min (&root->right_child);
          *root_ptr = successor;
          return root;
        }
      else
        {
          *root_ptr = root->left_child;
          return root;
        }
    }
  
  struct sleep_thread *parent;
  struct sleep_thread *tmp = root;

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
                  struct sleep_thread *successor = slpq_delete_min (&tmp->right_child);
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

struct sleep_thread *slpq_delete_min (struct sleep_thread **root_ptr)
{
  struct sleep_thread *root = *root_ptr;
  if (root == NULL)
    return NULL;
  struct sleep_thread *parent = root;
  struct sleep_thread *tmp = root;
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

struct sleep_thread *slpq_peek_min (struct sleep_thread *root)
{
  if (root == NULL)
    return NULL;
  struct sleep_thread *tmp = root;
  while (tmp->left_child != NULL)
    tmp = tmp->left_child;
  return tmp;
}

struct sleep_thread *slpq_pop_min (struct sleep_thread **root_ptr)
{
  struct sleep_thread *root = *root_ptr;
  if (root == NULL)
    return NULL;
  struct sleep_thread *tmp = root;
  while (tmp->left_child != NULL)
    {
      tmp = tmp->left_child;
    }
  slpq_delete (root_ptr, tmp);
  return tmp;
}

struct sleep_thread *slpq_update (struct sleep_thread **root_ptr,
 struct sleep_thread *node, int64_t new_tick)
{
  slpq_delete(root_ptr, node);
  node->final_tick = new_tick;
  struct sleep_thread *new_node = slpq_insert(root_ptr, node);
  return new_node;
}