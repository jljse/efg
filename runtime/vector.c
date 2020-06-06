#include "g_runtime.h"
#include <stdlib.h>

/* 単純にvectorから検索(使ってないけど) */
/*
static int g_vector_find(struct g_vector* v, void* p)
{
  int i;
  for(i=0; i<g_vector_size(v); ++i){
    if(g_vector_get(v,i) == p){
      return i;
    }
  }
  return -1;
}
*/

struct g_vector* g_vector_new()
{
  struct g_vector* result = malloc(sizeof(struct g_vector));
  result->size = 0;
  result->capacity = 4;
  result->data = malloc(sizeof(void*) * result->capacity);
  return result;
}

void g_vector_delete(struct g_vector* v)
{
  free(v->data);
  free(v);
}

/* マクロ化
void g_vector_set(struct g_vector* v, int n, void* x)
{
  assert(g_vector_size(v) > n);
  v->data[n] = x;
}
*/

/* マクロ化
void* g_vector_get(struct g_vector* v, int n)
{
  assert(g_vector_size(v) > n);
  return v->data[n];
}
*/

/* マクロ化
int g_vector_size(struct g_vector* v)
{
  return v->size;
}
*/

void g_vector_resize(struct g_vector* v, int x)
{
  if(v->size < x){
    /* 今のサイズより大きい場合 */
    if(v->capacity <= x){
      /* capacityよりも大きいと再確保の必要がある */
      /* 今のcapacityの何倍かに拡張する */
      int newsize = v->capacity * (x/v->capacity + 1);
      v->data = realloc(v->data, sizeof(void*) * newsize);
    }
    int i;
    for(i=v->size; i<x; ++i){
      /* 拡張された部分をNULLで初期化 */
      v->data[i] = NULL;
    }
  }
  /* 縮む場合は何もしない */
  
  v->size = x;
}

void g_vector_push(struct g_vector* v, void* x)
{
  int newsize = g_vector_size(v) + 1;
  g_vector_resize(v, newsize);
  g_vector_set(v, newsize-1, x);
}

