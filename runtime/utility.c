#include "utility.h"
#include <stdlib.h>

g_pair* g_pair_new(void* f, void* s)
{
  g_pair* result = malloc(sizeof(g_pair));
  result->first = f;
  result->second = s;
  return result;
}

void g_pair_init(g_pair* p, void* x, void* y)
{
  p->first = x;
  p->second = y;
}

void g_pair_delete(g_pair* p)
{
  free(p);
}

