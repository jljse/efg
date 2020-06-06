#include "p_to_c.h"

void p_to_c_context::regist_freelink(c_node::c_link* l)
{
  freelinks.push_back(l);
}

void p_to_c_context::regist_pcontext(c_node::c_pcontext* p)
{
  pcontexts.push_back(p);
}

void p_to_c_context::regist_atomic(c_node::c_atomic* a)
{
  atomics.push_back(a);
}

void p_to_c_context::regist_rule(c_node::c_rule* r)
{
  rules.push_back(r);
}

