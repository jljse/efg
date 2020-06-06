#ifndef COMPILER_H_INCLUDED
#define COMPILER_H_INCLUDED

#include "p_node.h"
#include "c_node.h"
#include "il_node.h"

class compiler
{
public:
  // パーズツリーをグラフ構造に変換する. 変換が失敗した場合NULLを返す
  c_node::c_root* to_c_node(const p_node::p_root* root);
  il_node::il_root* compile(const c_node::c_root* root);
};

#endif
