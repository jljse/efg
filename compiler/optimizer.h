#ifndef OPTIMIZER_H_INCLUDED
#define OPTIMIZER_H_INCLUDED

#include "il_node.h"

class optimizer
{
public:
  void optimize(il_node::il_root*);
};

#endif
