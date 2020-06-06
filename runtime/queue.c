#include "queue.h"
#include <stdlib.h>
#include <assert.h>

struct g_queue* g_queue_new(void)
{
  struct g_queue* result = malloc(sizeof(struct g_queue));
  result->next_push_pos = 0;
  result->next_pop_pos = 0;
  result->data = g_vector_new();
  g_vector_resize(result->data, 4); /* とりあえず初期サイズは4 */
  return result;
}

void g_queue_delete(struct g_queue* q)
{
  g_vector_delete(q->data);
  free(q);
}

void g_queue_push(struct g_queue* q, void* p)
{
  /* データをpushする */
  g_vector_set(q->data, q->next_push_pos, p);
  /* push場所を進める */
  q->next_push_pos = (q->next_push_pos+1) % g_vector_size(q->data);
  /* 一致したらいっぱいなので拡張 */
  if(q->next_push_pos == q->next_pop_pos){
    /* もうちょっと賢い拡張方法がありそうだけどとりあえず */
    int i;
    int old_size = g_vector_size(q->data);
    int new_size = old_size * 2;
    /* pop_pos から末尾までの部分のサイズ */
    int after_pop_pos_width = old_size - q->next_pop_pos;
    /* 2倍に拡張 */
    g_vector_resize(q->data, new_size);
    /* pop posから元の末尾までの部分を最後尾に移す (絶対重ならないけど一応後ろから) */
    for(i=0; i<after_pop_pos_width; ++i){
      /* 後ろからi個目の要素を移動 */
      g_vector_set(q->data, new_size-1-i, g_vector_get(q->data, old_size-1-i));
    }
    /* pop posを書き換える */
    q->next_pop_pos = new_size - after_pop_pos_width;
  }
}

void* g_queue_pop(struct g_queue* q)
{
  void* result;
  assert(! g_queue_empty(q));
  /* pop場所のデータを返す */
  result = g_vector_get(q->data, q->next_pop_pos);
  /* pop場所を1つ進める */
  q->next_pop_pos = (q->next_pop_pos+1) % g_vector_size(q->data);
  return result;
}

void* g_queue_peek(const struct g_queue* q)
{
  void* result;
  assert(! g_queue_empty(q));
  /* pop場所のデータを返す */
  result = g_vector_get(q->data, q->next_pop_pos);
  return result;
}

int g_queue_empty(const struct g_queue* q)
{
  /* 次のpop場所が次のpush場所と一致 -> キューは空 */
  return q->next_pop_pos == q->next_push_pos;
}

