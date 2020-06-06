#include "g_runtime.h"
#include "vector.h"
#include "tree.h"
#include "dumper.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

/* --------------------------
 * ここから古い実装
 * -------------------------- */

/* アトム1つだけを手抜き表示, linksはこれまでに表示したlink */
static void dump_atom_raw(struct g_atom* a, g_tree* vlinks)
{
  int i;
  const struct g_functor* functor = &g_functors[a->functor];
  
  printf("'%s' (", g_symbols[functor->symbol]);
  for(i=0; i<functor->arity; ++i){
    if(i != 0) printf(", ");
    struct g_link* l = a->args[i];
    /* l が既に表示されていないか調べる */
    g_tree_node* pos = g_tree_find(vlinks, l);
    int link_num;
    
    if(pos == g_tree_itor_end(vlinks)){
      /* 未表示なら lの相手側を登録(もうlが出てくることは無いため相手側だけでよい) */
      link_num = g_tree_size(vlinks);
      g_tree_insert(vlinks, l->buddy, (void*)(intptr_t)link_num);
    }else{
      /* 一度表示されていたらそのindexを使う */
      link_num = (int)(intptr_t)pos->val;
    }
    
    printf("L%d", link_num);
  }
  printf(").\n");
}

/* ツリー用の比較関数 左が先にくる場合は負を返す */
static int compare_normal(void* x, void* y);

/* 全アトムを手抜きに表示 */
void dump_atoms_raw(struct g_thread *th)
{
  int i;
  g_tree* vlinks = g_tree_new(compare_normal);
  
  for(i=0; i<g_functors_size; ++i){
    struct g_atom* a;
    for(a = g_atomlist_begin(th, i);
        a != g_atomlist_end(th, i);
        a = g_atomlist_next(a)){
      /* アトム1つを手抜き表示 */
      dump_atom_raw(a, vlinks);
    }
  }
  
  g_tree_delete(vlinks);
}


/* --------------------------
 * ここから新しい実装
 * -------------------------- */

static void dump_atom_inner(g_tree* vatoms, g_tree* vlinks, struct g_atom* a, int is_inplace);

/* アトムの表示優先度を返す 優先度が高いほど大きい値 */
static int calc_atom_priority(const struct g_atom* a)
{
  /* TODO: ちゃんと実装 */
  
  /* 今は 自分の最終引数が相手の最終引数以外につながっている (=相手側に埋め込みたい) 場合に0, そうでない場合に1を返す 引数がないものは2 */
  const struct g_functor* functor = &g_functors[a->functor];
  
  if(functor->arity == 0){
    return 2;
  }else{
    struct g_link* last_arg = a->args[functor->arity-1];
    struct g_atom* next = last_arg->buddy->atom;
    const struct g_functor* next_functor = &g_functors[next->functor];
  
    if(last_arg->buddy->pos != next_functor->arity-1){
      return 0;
    }else{
      return 1;
    }
  }
}

/* qsort用の比較関数(左が先なら負を返す) 要素のポインタなので注意 優先度の大きい順に並ぶはず */
static int compare_atom_priority(const void* p_of_x, const void* p_of_y)
{
  return calc_atom_priority(*(const struct g_atom**)p_of_y) - calc_atom_priority(*(const struct g_atom**)p_of_x);
}

/* 全アトムを始点として優先する順に並び替える */
static void sort_atoms_priority(struct g_vector* allatoms)
{
  /* 掟破りのvector内直接アクセス 優先度の大きい順に並べ替える */
  qsort(allatoms->data, g_vector_size(allatoms), sizeof(struct g_vector*), compare_atom_priority);
}

/* アトム名を出力 適当にクォート */
static void dump_atom_name(const struct g_functor* functor)
{
  /* TODO: クォート */
  printf("%s", g_symbols[functor->symbol]);
}

/* リンクのbuddy側を適切な形で出力 */
static void dump_link(g_tree* vatoms, g_tree* vlinks, struct g_link* l)
{
  /* buddy側につながった未出力かもしれないアトム */
  struct g_atom* next;
  /* そのファンクタ */
  const struct g_functor* next_functor;
  
  /* すでにそのリンクが出力されていれば,その名前を使う */
  {
    g_tree_node* found = g_tree_find(vlinks, l);
    if(found != g_tree_itor_end(vlinks)){
      /* リンクが出力済みなのでその名前を出力 */
      printf("L%d", (int)(intptr_t)found->val);
      return;
    }
  }
  
  /* その先のアトムが出力済みなら,またはその先のアトムの最終引数以外につながっているなら, リンクの形でしか出力できない */
  next = l->buddy->atom;
  next_functor = &g_functors[next->functor];
  {
    g_tree_node* found = g_tree_find(vatoms, next);
    if(found!=g_tree_itor_end(vatoms) || l->buddy->pos!=next_functor->arity-1){
      /* アトムが出力済み or 最終引数以外につながっている ので新しいリンク名として出力 + そのリンク名を登録 */
      int link_num = g_tree_size(vlinks);
      printf("L%d", link_num);
      g_tree_insert(vlinks, l->buddy, (void*)(intptr_t)link_num);
      return;
    }
  }
  
  /* アトムはここで表示するので登録(keyのみでいい) */
  g_tree_insert(vatoms, next, NULL);
  
  /* 特殊な出力が可能ならここでする */
  
  /* TODO: リストとか全部ここで 特殊ファンクタのテーブルを用意するか,generate側で予約語的に扱わないとダメ */
  
  /* それ以外の場合,普通に出力 */
  dump_atom_inner(vatoms, vlinks, next, 1);
}

/* aを出力, is_inplace==trueなら何かの子としての出力なので最後の引数は出力しない */
/* aは未出力であり,直前に登録済みのアトム */
static void dump_atom_inner(g_tree* vatoms, g_tree* vlinks, struct g_atom* a, int is_inplace)
{
  const struct g_functor* functor = &g_functors[a->functor];
  /* 出力対象の引数の個数 is_inplace==true なら最後の引数は出さない */
  int arg_size = functor->arity - ((is_inplace) ? 1 : 0);
  
  /* アトム名を出力 */
  dump_atom_name(functor);
  
  if(arg_size > 0){
    int i;
    printf("(");
    for(i=0; i<arg_size; ++i){
      if(i != 0) printf(",");
      /* 各引数を出力 */
      dump_link(vatoms, vlinks, a->args[i]);
    }
    printf(")");
  }
}

/* aをトップレベルとして出力 */
static void dump_atom_toplevel(g_tree* vatoms, g_tree* vlinks, struct g_atom* a)
{
  g_tree_insert(vatoms, a, NULL); /* keyで既出かどうかだけ分かればいいのでvalは不要 */
  dump_atom_inner(vatoms, vlinks, a, 0);
}

/* 単に比較するだけ xをyより先に整列するなら負を返す */
static int compare_normal(void* x, void* y)
{
  return x - y;
}

/* 全アトムを表示 */
void dump_atoms(struct g_thread *th)
{
  int i;
  int is_something_dumped = 0;
  /* 全アトムの配列 */
  struct g_vector* allatoms = g_vector_new();
  /* 出力済みアトム */
  g_tree* vatoms = g_tree_new(compare_normal);
  /* 出力済みリンク 出力したリンクのbuddy側を登録する(同じリンク端が2回出てくることは無いので自分側を登録しても無駄) */
  g_tree* vlinks = g_tree_new(compare_normal);
  
  /* 全アトムを集める */
  for(i=0; i<g_functors_size; ++i){
    struct g_atom* a;
    for(a = g_atomlist_begin(th, i);
        a != g_atomlist_end(th, i);
        a = g_atomlist_next(a)){
      g_vector_push(allatoms, a);
    }
  }

  /* 全アトムを優先度別に並び替える */
  sort_atoms_priority(allatoms);
  
  /* それぞれのアトムを根として出力 */
  for(i=0; i<g_vector_size(allatoms); ++i){
    struct g_atom* a = g_vector_get(allatoms, i);
    
    if(g_tree_find(vatoms, a) == g_tree_itor_end(vatoms)){
      /* aが未出力アトムなら */
      if(is_something_dumped){
        /* 何かその前に出力してあれば区切りが必要 */
        printf(", ");
      }
      dump_atom_toplevel(vatoms, vlinks, a);
      is_something_dumped = 1;
    }
  }
  printf(".\n");
  
  g_tree_delete(vatoms);
  g_tree_delete(vlinks);
  g_vector_delete(allatoms);
}
