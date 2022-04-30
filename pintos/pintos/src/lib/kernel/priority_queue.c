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


