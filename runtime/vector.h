#ifndef VECTOR_H_INCLUDED
#define VECTOR_H_INCLUDED

/* ポインタのベクタ */
struct g_vector
{
  int size;
  int capacity;
  void **data;
};

/* ベクタを生成 */
struct g_vector* g_vector_new();
/* ベクタを削除 */
void g_vector_delete(struct g_vector*);
/* ベクタの要素にset */
/* void g_vector_set(struct g_vector*, int, void*); */
#define g_vector_set(V,N,X)  ((V)->data[N]=(X))
/* ベクタの要素をget */
/* void* g_vector_get(struct g_vector*, int); */
#define g_vector_get(V,N)  ((V)->data[N])
/* ベクタのサイズ */
/* int g_vector_size(struct g_vector*); */
#define g_vector_size(V)  ((V)->size)
/* ベクタのリサイズ(拡張分はNULL) */
void g_vector_resize(struct g_vector*, int);
/* ベクタの末尾に追加 */
void g_vector_push(struct g_vector*, void*);

#endif
