struct thread_node *
tnpq_insert (struct thread_node *root,
 struct thread_node *node)
{
  if(root == NULL) // Tree is empty
    {
        root = node;
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
      parent = tmp;
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
      *root = tmp->right_child;
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
  while (tmp->left_child != NULL)
    tmp = tmp->left_child;
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