#include "g_runtime.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "memory_pool.h"
#include "utility.h"
#include "tree.h"
#include "dumper.h"

/* メモリプールの集合 */
struct g_vector* memory_pools;
/* 中断キュー */
struct g_queue* tasks;
/* アトムリスト */
struct g_vector* atoms;


struct g_atom* g_atomlist_begin(struct g_thread *th, int f)
{
  g_pair* p = g_vector_get(th->atoms, f);
  struct g_atom* a = p->first;
  /* 一番最初は番兵なので次を返す */
  return a->next;
}

struct g_atom* g_atomlist_end(struct g_thread *th, int f)
{
  g_pair* p = g_vector_get(th->atoms, f);
  return p->second;
}

struct g_atom* g_atomlist_next(struct g_atom* a)
{
  return a->next;
}

/* 内部のメモリプールを使ったmalloc サイズ0の要素の要求は1ワード分返す */
/* マクロ化
static void* pooled_malloc(struct g_thread *th, size_t t)
{
  memory_pool* mp;
  if(! (t/4 < g_vector_size(th->memory_pools))){
    assert(t/4 < g_vector_size(th->memory_pools)); /\* TODO: サイズの要素は4の倍数 *\/
  }
  
  mp = g_vector_get(th->memory_pools, t/4);
  return memory_pool_malloc(mp);
}
*/

/* 内部のメモリプールを使ったfree サイズ0の要素に対しては1ワード分返す */
/* マクロ化
static void pooled_free(struct g_thread *th, size_t t, void* p)
{
  memory_pool* mp;
  assert(t/4 < g_vector_size(th->memory_pools)); /\* TODO: サイズの要素は4の倍数 *\/
  
  mp = g_vector_get(th->memory_pools, t/4);
  memory_pool_free(mp, p);
}
*/

struct g_atom* g_newatom(struct g_thread *th, int f)
{
  int arity = g_functors[f].arity;
  /* そのファンクタのヘッドアトム */
  struct g_atom* head = ((g_pair*)g_vector_get(th->atoms, f))->first;
  struct g_atom* result = pooled_malloc(th, sizeof(struct g_atom));
  struct g_link** args = pooled_malloc(th, sizeof(struct g_link) * arity);
  result->functor = f;
  result->args = args;
  result->is_queued = 0;
  /* アトムリストに挿入 */
  head->next->prev = result;
  result->next = head->next;
  head->next = result;
  result->prev = head;
  return result;
}

struct g_link* g_newlink(struct g_thread *th)
{
  struct g_link* result = pooled_malloc(th, sizeof(struct g_link));
  result->pos = -1; /* つながっていないことを表す値 */
  result->atom = 0;
  return result;
}

/* マクロ化
void g_freelink(struct g_thread *th, struct g_link* l)
{
  pooled_free(th, sizeof(struct g_link), l);
}
*/

void g_freeatom(struct g_thread *th, struct g_atom* a)
{
  /* すでに消えているアトムの抜け殻を消す場合 */
  if(a->functor == -1){
    if(! a->is_queued){
      /* 再実行キュー上になければ消せる */
      pooled_free(th, sizeof(struct g_atom), a);
    }else{
      /* アトムの抜け殻が再実行キューに乗ったままの状態で削除されることはない */
      assert(0);
    }
  }
  /* まだ消えていないアトムを消す場合 */
  else{
    /* 前後のアトムをつなぐ */
    a->prev->next = a->next;
    a->next->prev = a->prev;

    /* 引数部分はすぐ消せる */
    pooled_free(th, sizeof(struct g_link) * g_functors[a->functor].arity, a->args);
    
    if(a->is_queued){
      /* 再実行キュー上にある場合は抜け殻として残しておく */
      a->functor = -1;
    }else{
      /* 再実行キュー上になければ全部消せる */
      pooled_free(th, sizeof(struct g_atom), a);
    }
  }
}

/* マクロ化
struct g_link* g_getlink(struct g_atom* a, int n)
{
  return a->args[n];
}
*/

/* マクロ化
void g_setlink(struct g_atom* a, int n, struct g_link* l)
{
  a->args[n] = l;
  l->atom = a;
  l->pos = n;
}
*/

void g_enqueue(struct g_thread *th, struct g_atom* a)
{
  /* printf("enqueue %s:%d\n", g_symbols[g_functors[a->functor].symbol], g_functors[a->functor].arity); */
  g_queue_push(th->tasks, a);
  a->is_queued = 1;
}

/* マクロ化
void g_commit(void)
{
  /\* printf("commit!\n"); *\/
}
*/

/* タスクキューを用意する (newするだけ) */
static void init_tasks()
{
  tasks = g_queue_new();
}

/* アトムリストを用意する (newして各ファンクタに先頭と末尾の番兵を入れておく */
static void init_atoms()
{
  int i;
  
  atoms = g_vector_new();
  g_vector_resize(atoms, g_functors_size);

  for(i=0; i<g_functors_size; ++i){
    /* 先頭側の番兵 */
    struct g_atom* begin = malloc(sizeof(struct g_atom));
    /* 終端側の番兵 */
    struct g_atom* end = malloc(sizeof(struct g_atom));
    g_pair* pair = g_pair_new(begin, end);
    begin->next = end;
    end->prev = begin;
    g_vector_set(atoms, i, pair);
  }
}

/* メモリプールを用意する */
static void init_memory_pools()
{
  int i;
  memory_pools = g_vector_new();
  g_vector_resize(memory_pools, 32); /* TODO: とりあえず32wordまで確保 */
  for(i=0; i<g_vector_size(memory_pools); ++i){
    g_vector_set(memory_pools, i, memory_pool_new(i*4)); /* TODO: 要素サイズが4の倍数 */
  }
}

/* タスクキューを削除 */
static void finit_tasks()
{
  g_queue_delete(tasks);
}

/* アトムリストを削除 */
static void finit_atoms()
{
  int i;
  for(i=0; i<g_functors_size; ++i){
    g_pair_delete(g_vector_get(atoms, i));
  }
  g_vector_delete(atoms);
}

/* メモリプールを削除 */
static void finit_memory_pools()
{
  int i;
  for(i=0; i<g_vector_size(memory_pools); ++i){
    memory_pool_delete(g_vector_get(memory_pools, i));
  }
  g_vector_delete(memory_pools);
}

/* aを始点とする書き換えを再開する */
static int resume_reduction(struct g_thread *th, struct g_atom* a)
{
  /* TODO: 引数をうまく処理する手段はないものか... */
  
  const struct g_functor* f = &g_functors[a->functor];
  int result;
  
  switch(f->arity){
  case 0:
    {
      g_freeatom(th, a);
      result = (*f->function)(th);
    }
    break;
  case 1:
    {
      struct g_link* arg0 = a->args[0];
      g_freeatom(th, a);
      result = (*f->function)(th, arg0);
    }
    break;
  case 2:
    {
      struct g_link* arg0 = a->args[0];
      struct g_link* arg1 = a->args[1];
      g_freeatom(th, a);
      result = (*f->function)(th, arg0, arg1);
    }
    break;
  case 3:
    {
      struct g_link* arg0 = a->args[0];
      struct g_link* arg1 = a->args[1];
      struct g_link* arg2 = a->args[2];
      g_freeatom(th, a);
      result = (*f->function)(th, arg0, arg1, arg2);
    }
    break;
  case 4:
    {
      struct g_link* arg0 = a->args[0];
      struct g_link* arg1 = a->args[1];
      struct g_link* arg2 = a->args[2];
      struct g_link* arg3 = a->args[3];
      g_freeatom(th, a);
      result = (*f->function)(th, arg0, arg1, arg2, arg3);
    }
    break;
  case 5:
    {
      struct g_link* arg0 = a->args[0];
      struct g_link* arg1 = a->args[1];
      struct g_link* arg2 = a->args[2];
      struct g_link* arg3 = a->args[3];
      struct g_link* arg4 = a->args[4];
      g_freeatom(th, a);
      result = (*f->function)(th, arg0, arg1, arg2, arg3, arg4);
    }
    break;
  case 6:
    {
      struct g_link* arg0 = a->args[0];
      struct g_link* arg1 = a->args[1];
      struct g_link* arg2 = a->args[2];
      struct g_link* arg3 = a->args[3];
      struct g_link* arg4 = a->args[4];
      struct g_link* arg5 = a->args[5];
      g_freeatom(th, a);
      result = (*f->function)(th, arg0, arg1, arg2, arg3, arg4, arg5);
    }
    break;
  case 7:
    {
      struct g_link* arg0 = a->args[0];
      struct g_link* arg1 = a->args[1];
      struct g_link* arg2 = a->args[2];
      struct g_link* arg3 = a->args[3];
      struct g_link* arg4 = a->args[4];
      struct g_link* arg5 = a->args[5];
      struct g_link* arg6 = a->args[6];
      g_freeatom(th, a);
      result = (*f->function)(th, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    }
    break;
  case 8:
    {
      struct g_link* arg0 = a->args[0];
      struct g_link* arg1 = a->args[1];
      struct g_link* arg2 = a->args[2];
      struct g_link* arg3 = a->args[3];
      struct g_link* arg4 = a->args[4];
      struct g_link* arg5 = a->args[5];
      struct g_link* arg6 = a->args[6];
      struct g_link* arg7 = a->args[7];
      g_freeatom(th, a);
      result = (*f->function)(th, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    break;
  case 9:
    {
      struct g_link* arg0 = a->args[0];
      struct g_link* arg1 = a->args[1];
      struct g_link* arg2 = a->args[2];
      struct g_link* arg3 = a->args[3];
      struct g_link* arg4 = a->args[4];
      struct g_link* arg5 = a->args[5];
      struct g_link* arg6 = a->args[6];
      struct g_link* arg7 = a->args[7];
      struct g_link* arg8 = a->args[8];
      g_freeatom(th, a);
      result = (*f->function)(th, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    break;
  default:
    /* 引数が多すぎて再開できない */
    fprintf(stderr, "implimentation error: cannot resume reduction(too much arguments)\n");
    assert(0);
    break;
  }
  
  return result;
}

int main()
{
  /* リダクションが発生したかどうか覚えておくフラグ */
  int is_reduction_occured = 0;

  /* メモリプールを用意する */
  init_memory_pools();
  /* タスクキューを用意する */
  init_tasks();
  /* アトムリストを用意する */
  init_atoms();
  
  /* TODO: スレッド情報 */
  struct g_thread *th = malloc(sizeof(struct g_thread));
  th->memory_pools = memory_pools;
  th->tasks = tasks;
  th->atoms = atoms;
  
  /* 生成されたメイン関数 */
  gr__main(th);
  /* 一度実行が終わったところで,1周を検知するための目印(NULL)を入れる */
  g_queue_push(th->tasks, NULL);
  
  while(1){
    struct g_atom* a = g_queue_pop(th->tasks);
    
    if(! a){
      /* 1周したことを検知するために入れてあるNULLなら */
      if(is_reduction_occured){
        /* もしリダクションが発生していれば引き続き実行 */
        g_queue_push(th->tasks, NULL);
        is_reduction_occured = 0;
      }else{
        /* リダクションが発生しないまま一周したら終了 */
        break;
      }
    }else{
      /* 通常の中断アトムなら */
      a->is_queued = 0;
    
      if(a->functor == -1){
        /* すでに消されたアトムなら単に消す */
        /* TODO: argsもstructに埋め込んだ場合ファンクタの情報がなくなるとpooled_freeできないから別の方法を考える */
        g_freeatom(th, a);
      }else{
        /* まだ消されていないアトムなら再実行 */
	/* TODO: スレッド情報 */
        if( resume_reduction(th, a) ){
          /* 書き換えが発生したらフラグを立てる */
          is_reduction_occured = 1;
        }
      }
    }
  }

  /* 結果を出力 */
  dump_atoms(th);

  finit_atoms();
  finit_tasks();
  finit_memory_pools();
  
  return 0;
}

