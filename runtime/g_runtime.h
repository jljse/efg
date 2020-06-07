#ifndef G_RUNTIME_H_INCLUDED
#define G_RUNTIME_H_INCLUDED

enum g_reserved_functor
{
    /* 削除済み */
    G_FUNCTOR_DELETED = -1,
};

enum g_reserved_pos
{
    /* 整数 */
    G_RESERVED_POS_INTEGER = -1,
};

/* ファンクタ構造体 */
struct g_functor
{
  int symbol;
  int arity;
  /* そのファンクタ用の関数ポインタ */ 
  int (*function)();
};

struct g_atom;

/* リンク */
struct g_link
{
  struct g_link* buddy;
  struct g_atom* atom;
  int pos; /* データ埋め込みの場合posが負になる */
};

/* アトム */
struct g_atom
{
  int functor; /* functor == -1なら削除されたアトム(再実行の際のみ可能性がある) */
  struct g_link** args;
  /* 前後のアトムへのポインタ */
  struct g_atom* prev;
  struct g_atom* next;
  /* 中断キュー上にあるかどうか  */
  int is_queued;
};

struct g_vector;
struct g_queue;

/* スレッド情報 */
struct g_thread
{
  /* このスレッド用のメモリプール */
  struct g_vector* memory_pools;
  /* このスレッドにおける一時停止アトム */
  struct g_queue* tasks;
  /* このスレッドが所有するアトムリスト */
  struct g_vector* atoms;
};
  

/* あるファンクタのアトムを作る */
struct g_atom* g_newatom(struct g_thread*, int);
/* リンクを作る */
struct g_link* g_newlink(struct g_thread*);
/* リンクを消す */
/* void g_freelink(struct g_thread*, struct g_link*); */
#define g_freelink(TH,L)  pooled_free(TH,sizeof(struct g_link),L)
/* アトムを消す */
void g_freeatom(struct g_thread*, struct g_atom*);
/* アトムのある引数リンクを取得する */
/* struct g_link* g_getlink(struct g_atom*, int); */
#define g_getlink(A,N)  ((A)->args[N])
/* アトムのある引数として,アトムとリンクを設定する */
/* void g_setlink(struct g_atom*, int, struct g_link*); */
#define g_setlink(A,N,L)  do{(A)->args[N]=L; (L)->atom=A; (L)->pos=N;}while(0)
/* 整数かどうか判定 */
int g_isint(struct g_link*);
/* 整数を取得 */
int g_getint(struct g_link*);
/* 整数を設定 */
void g_setint(struct g_link*, int);
/* 実行が中断したアトムを後回しにする */
void g_enqueue(struct g_thread*, struct g_atom*);
/* あるファンクタのイテレータを取得 */
struct g_atom* g_atomlist_begin(struct g_thread*, int);
/* あるファンクタの終端イテレータを取得 */
struct g_atom* g_atomlist_end(struct g_thread*, int);
/* アトムリスト上の次を取得 */
struct g_atom* g_atomlist_next(struct g_atom*);
/* コミット */
/* void g_commit(void); */
#define g_commit() 

/* --------------------------
 * ブリッジになる変数
 * -------------------------- */
void gr__main(struct g_thread*);
extern const int g_symbols_size;
extern const char * const g_symbols[];
extern const int g_functors_size;
extern const struct g_functor g_functors[];

/* --------------------------
 * マクロ展開のためだけに見せる必要がある内部の実装(本来は外に見せる必要なし,外からも使わない)
 * -------------------------- */
#include "vector.h"
#include "queue.h"
#include "memory_pool.h"
#define pooled_malloc(TH,T) memory_pool_malloc(g_vector_get((TH)->memory_pools,(T)/4))
#define pooled_free(TH,T,P) memory_pool_free(g_vector_get((TH)->memory_pools,(T)/4),P)
extern struct g_vector* memory_pools;
extern struct g_queue* tasks;
extern struct g_vector* atoms;

#endif

