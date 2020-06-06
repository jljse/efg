#ifndef QUEUE_H_INCLUDED
#define QUEUE_H_INCLUDED

#include "vector.h"

/* ポインタのキュー (実装はリングバッファ) */
struct g_queue
{
   /* 次プッシュするindex */
  int next_push_pos;
  /* 次ポップするindex */
  int next_pop_pos;
  struct g_vector* data;
};

/* キューを生成 */
struct g_queue* g_queue_new(void);
/* キューを削除 */
void g_queue_delete(struct g_queue*);
/* キューに追加 */
void g_queue_push(struct g_queue* q, void*);
/* キューから取り出す */
void* g_queue_pop(struct g_queue*);
/* キューから取り出さずに見る */
void* g_queue_peek(const struct g_queue*);
/* キューが空かどうか */
int g_queue_empty(const struct g_queue*);

#endif
