#ifndef P_TO_C_H_INCLUDED
#define P_TO_C_H_INCLUDED

#include <vector>
#include "c_node.h"

class p_to_c_context
{
public:
  std::vector<c_node::c_link*> freelinks;
  std::vector<c_node::c_pcontext*> pcontexts;
  std::vector<c_node::c_atomic*> atomics;
  std::vector<c_node::c_rule*> rules;

  // 出現した自由リンクの登録
  void regist_freelink(c_node::c_link* l);
  // 出現したプロセス文脈の登録
  void regist_pcontext(c_node::c_pcontext* p);
  // 出現したアトムの登録(プロセス文脈は除く)
  void regist_atomic(c_node::c_atomic* a);
  // 出現したルールの登録
  void regist_rule(c_node::c_rule* r);
};

#endif
