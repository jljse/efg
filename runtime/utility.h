#ifndef UTILITY_H_INCLUDED
#define UTILITY_H_INCLUDED

/* ポインタ2つ */
struct g_pair_
{
  void* first;
  void* second;
};

typedef struct g_pair_ g_pair;
/* ペアをヒープ上に確保 */
g_pair* g_pair_new(void* f, void* s);
/* 確保済みのペアを初期化 */
void g_pair_init(g_pair* p, void* x, void* y);
/* ヒープ上のペアを開放 */
void g_pair_delete(g_pair* p);

#endif
