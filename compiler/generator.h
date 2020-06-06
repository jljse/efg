#ifndef GENERATOR_H_INCLUDED
#define GENERATOR_H_INCLUDED

#include <ostream>
#include "il_node.h"

class generator
{
public:
  // 中間命令ツリーをソースコードとして出力する
  void generate(const il_node::il_root* in, std::ostream& out);
};

#endif
