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
        root->next = NULL;
    }
  else
    {
      struct thread *tmp = root;
      /* Find the place of node in the tree */
      while (tmp->next != NULL)
        {
          tmp = tmp->next;
        }
      /* Insert and set the pointers */
      tmp->next = node;
      node->next = NULL;
    }
    
    return root;
}

struct thread *tnpq_search (struct thread *root,
 struct thread *node)
{
  if (root == NULL)
    return NULL;
  
  struct thread *tmp = root;
  /* Search the list */
  while (tmp != NULL)
    {
      if (tmp == node)
        return node;
      tmp = tmp->next;
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
  struct thread *tmp = root;
  if (root->next == NULL)
    {
      *root_ptr = NULL;
      return root;
    }
  if (node == root)
    {
      *root_ptr = root->next;
      return node;
    }
  while (tmp && tmp->next != node)
    {
      tmp = tmp->next;
    }
  if (tmp->next)
    {
      tmp->next = tmp->next->next;
      return node;
    }
  return NULL; // Could not find the node
}


struct thread *tnpq_peek_max (struct thread *root)
{
  if (root == NULL)
    return NULL;
  int max_eff = -1;
  struct thread *max_thread = root;
  struct thread *tmp = root;
  while (tmp != NULL)
    {
      if (tmp->effective_priority > max_eff)
        {
          max_eff = tmp->effective_priority;
          max_thread = tmp;
        }
      tmp = tmp->next;
    }
  return max_thread;
}

struct thread *tnpq_pop_max (struct thread **root_ptr)
{
  struct thread *root = *root_ptr;
  if (root == NULL)
    return NULL;
  int max_eff = -1;
  struct thread *max_thread = root;
  struct thread *tmp = root;
  while (tmp != NULL)
    {
      if (tmp->effective_priority > max_eff)
        {
          max_eff = tmp->effective_priority;
          max_thread = tmp;
        }
      tmp = tmp->next;
    }
  tnpq_delete (root_ptr, max_thread);
  return max_thread;
}



struct thread *
snpq_insert (struct thread **root_ptr,
 struct thread *node)
{
  struct thread *root = *root_ptr;
  if(root == NULL) // Tree is empty
    {
        *root_ptr = node;
        root = *root_ptr;
        root->sema_next = NULL;
    }
  else
    {
      struct thread *tmp = root;
      /* Find the place of node in the tree */
      while (tmp->sema_next != NULL)
        {
          tmp = tmp->sema_next;
        }
      /* Insert and set the pointers */
      tmp->sema_next = node;
      node->sema_next = NULL;
    }
    
    return root;
}

struct thread *snpq_search (struct thread *root,
 struct thread *node)
{
  if (root == NULL)
    return NULL;
  
  struct thread *tmp = root;
  /* Search the list */
  while (tmp != NULL)
    {
      if (tmp == node)
        return node;
      tmp = tmp->sema_next;
    }
  return NULL; // Could not find the node
}

struct thread *snpq_delete (struct thread **root_ptr,
 struct thread *node)
{
  struct thread *root = *root_ptr;

  /* Special case of empty tree */
  if (root == NULL)
    return NULL;
  
  /* Special case of deleting root */
  struct thread *tmp = root;
  if (root->sema_next == NULL)
    {
      *root_ptr = NULL;
      return root;
    }
  if (node == root)
    {
      *root_ptr = root->sema_next;
      return node;
    }
  while (tmp && tmp->sema_next != node)
    {
      tmp = tmp->sema_next;
    }
  if (tmp->sema_next)
    {
      tmp->sema_next = tmp->sema_next->sema_next;
      return node;
    }
  return NULL; // Could not find the node
}


struct thread *snpq_peek_max (struct thread *root)
{
  if (root == NULL)
    return NULL;
  int max_eff = -1;
  struct thread *max_thread = root;
  struct thread *tmp = root;
  while (tmp != NULL)
    {
      if (tmp->effective_priority > max_eff)
        {
          max_eff = tmp->effective_priority;
          max_thread = tmp;
        }
      tmp = tmp->sema_next;
    }
  return max_thread;
}

struct thread *snpq_pop_max (struct thread **root_ptr)
{
  struct thread *root = *root_ptr;
  if (root == NULL)
    return NULL;
  int max_eff = -1;
  struct thread *max_thread = root;
  struct thread *tmp = root;
  while (tmp != NULL)
    {
      if (tmp->effective_priority > max_eff)
        {
          max_eff = tmp->effective_priority;
          max_thread = tmp;
        }
      tmp = tmp->sema_next;
    }
  snpq_delete (root_ptr, max_thread);
  return max_thread;
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


