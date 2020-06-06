#ifndef TREE_H_INCLUDED
#define TREE_H_INCLUDED

/* ツリーのノード (mapの実装に使用) */
struct g_tree_node_
{
  struct g_tree_node_* parent;
  struct g_tree_node_* left;
  struct g_tree_node_* right;
  void* key;
  void* val;
};

typedef struct g_tree_node_ g_tree_node;
g_tree_node* g_tree_node_new();
void g_tree_node_delete(g_tree_node* n);

/* ツリー全体 */
struct g_tree_
{
  /* キーの比較関数 */
  int (*compare)(void*,void*);
  int size;
  g_tree_node* root;
};

typedef struct g_tree_ g_tree;
/* キーの比較関数を渡す 左側の引数が先になる場合負, 右側の引数が先になる場合正, 等しいと0 */
g_tree* g_tree_new(int (*compare)(void*,void*));
void g_tree_delete(g_tree* t);
int g_tree_size(g_tree* t);
/* ツリー内のノードを昇順で訪問する始点 */
g_tree_node* g_tree_itor_begin(g_tree* t);
/* ツリー内のノードを昇順で訪問する無効な終点 */
g_tree_node* g_tree_itor_end(g_tree* t);
/* イテレータを一つ進める */
g_tree_node* g_tree_itor_next(g_tree_node* current);
/* キーが一致する箇所を探す(存在しない場合itor_endの値になる) */
g_tree_node* g_tree_find(g_tree* t, void* key);
/* キーが一致する要素を削除する(存在しない場合は何もしない) */
/*TODO: void g_tree_erase_with_key(g_tree* t, void* key); */
/* イテレータが指す要素を削除する 戻り値は次の要素のイテレータ */
/*TODO: g_tree_node* g_tree_erase_with_itor(g_tree* t, g_tree_node* it); */
/* あるキーの値を設定する(既にあるかどうかは関係なし) */
void g_tree_insert(g_tree* t, void* key, void* val);

#endif
