#include "tree.h"
#include "utility.h"
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

g_tree_node* g_tree_node_new()
{
  g_tree_node* result = malloc(sizeof(g_tree_node));
  result->parent = 0;
  result->left = 0;
  result->right = 0;
  result->key = 0;
  result->val = 0;
  return result;
}

void g_tree_node_delete(g_tree_node* n)
{
  free(n);
}

int g_tree_size(g_tree* t)
{
  return t->size;
}

g_tree_node* g_tree_itor_begin(g_tree* t)
{
  g_tree_node* result = t->root;
  /* できるだけ左側のノードまで飛ばす */
  while(result->left) result=result->left;
  return result;
}

g_tree_node* g_tree_itor_end(g_tree* t)
{
  return t->root;
}

g_tree_node* g_tree_itor_next(g_tree_node* current)
{
  /* 右側があるなら次は右側 */
  if(current->right){
    current = current->right;
    /* できるだけ左側のノードまで飛ばす */
    while(current->left) current=current->left;
    return current;
  }
  /* 右側がないなら訪問済みのノードを遡る */
  while(current->parent->right == current){
    current = current->parent;
  }
  /* 左上に遡れなくなったら,その一つ右上が次のノード */
  return current->parent;
}

/* keyが存在する(または存在すべき)ノードの 親 とその右か左かのペアを返す */
static g_pair g_tree_find_pos_inner(g_tree* t, g_tree_node* parent, int r_or_l, void* key)
{
  g_tree_node* current;
  if(r_or_l){
    current = parent->right;
  }else{
    current = parent->left;
  }
  
  if(! current){
    g_pair result;
    g_pair_init(&result, parent, (void*)(intptr_t)r_or_l);
    return result;
  }else{
    /* 今のノードと探すキーを比較 左側が先なら負 */
    int compare_result = (*t->compare)(current->key, key);
    if(compare_result < 0){
      /* もし負なら左が先->探しているキーは今のノードより大きい */
      return g_tree_find_pos_inner(t, current, 1, key);
    }else if(compare_result > 0){
      return g_tree_find_pos_inner(t, current, 0, key);
    }else{
      g_pair result;
      g_pair_init(&result, parent, (void*)(intptr_t)r_or_l);
      return result;
    }
  }
}

static g_pair g_tree_find_pos(g_tree* t, void* key)
{
  /* rootの左側 */
  return g_tree_find_pos_inner(t, t->root, 0, key);
}

g_tree_node* g_tree_find(g_tree* t, void* key)
{
  g_pair temp = g_tree_find_pos(t, key);
  g_tree_node* parent = temp.first;
  int r_or_l = (int)(intptr_t)temp.second;
  if(r_or_l){
    if(parent->right){
      return parent->right;
    }else{
      return g_tree_itor_end(t);
    }
  }else{
    if(parent->left){
      return parent->left;
    }else{
      return g_tree_itor_end(t);
    }
  }
}

/* void g_tree_erase_with_key(g_tree* t, void* key)
 * {
 * }
 *
 * g_tree_node* g_tree_erase_with_itor(g_tree* t, g_tree_node* it)
 * {
 *   assert(it);
 *   assert(it != g_tree_itor_end(t));
 *   
 *   return 0;
 * }
 */

void g_tree_insert(g_tree* t, void* key, void* val)
{
  g_pair found = g_tree_find_pos(t, key);
  g_tree_node* parent = found.first;
  int r_or_l = (int)(intptr_t)found.second;
  
  if(r_or_l){
    /* 右側 */
    if(parent->right){
      /* ノードがあればそこに設定 */
      parent->right->val = val;
    }else{
      /* ノードが無ければ追加 */
      g_tree_node* current = g_tree_node_new();
      parent->right = current;
      current->parent = parent;
      current->key = key;
      current->val = val;
    }
  }else{
    /* 左側 */
    if(parent->left){
      parent->left->val = val;
    }else{
      g_tree_node* current = g_tree_node_new();
      parent->left = current;
      current->parent = parent;
      current->key = key;
      current->val = val;
    }
  }
  
  t->size = t->size+1;
}

g_tree* g_tree_new(int (*compare)(void*,void*))
{
  g_tree* result = malloc(sizeof(g_tree));
  result->compare = compare;
  result->root = g_tree_node_new();
  result->size = 0;
  return result;
}

static void g_tree_delete_inner(g_tree_node* n)
{
  if(n->left) g_tree_delete_inner(n->left);
  if(n->right) g_tree_delete_inner(n->right);
  g_tree_node_delete(n);
}

void g_tree_delete(g_tree* t)
{
  g_tree_delete_inner(t->root);
  free(t);
}

