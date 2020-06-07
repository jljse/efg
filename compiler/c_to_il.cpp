#include <utility>
#include "c_to_il.h"
#include "c_node.h"
#include "il_node.h"

using namespace std;

int matching_context::get_index(const c_node::c_atomic* x) const
{
  found_atom_type::const_iterator it = found_atom.find(x);
  if(it == found_atom.end()){
    return -1;
  }else{
    return it->second;
  }
}

int matching_context::get_index(const c_node::c_link* x) const
{
  found_link_type::const_iterator it = found_link.find(x);
  if(it == found_link.end()){
    return -1;
  }else{
    return it->second;
  }
}

int matching_context::add_index()
{
  int result = next_index++;
  return result;
}

int matching_context::add_index(const c_node::c_atomic* x)
{
  int result = next_index++;
  found_atom.insert(make_pair(x, result));
  return result;
}

int matching_context::add_index(const c_node::c_link* x)
{
  int result = next_index++;
  found_link.insert(make_pair(x, result));
  return result;
}

bool matching_context::is_isbuddy_checked(const c_node::c_link* x)
{
  return isbuddy_checked.find(x) == isbuddy_checked.end();
}

void matching_context::add_isbuddy_checked(const c_node::c_link* x)
{
  isbuddy_checked.insert(x);
}

void output_il_context::begin_rule()
{
  // ���Υ롼�뤬�����Ƚ�λ���Ƥ��ʤ�
  assert(block_stack.size() == 0);

  // result�˿�����������롼���1�����䤹
  result.push_back(new list<il_node::il_operator*>());
  // ������֥��å��Ȥ��ƺ����䤷���롼���Ȥ�
  block_stack.push(result.back());
}

void output_il_context::end_rule()
{
  // �֥��å��������Ĥ��뤿��ˤޤä����stack�Ǿ�񤭤���
  block_stack = stack<list<il_node::il_operator*>*>();
}

void output_il_context::output(il_node::il_operator* op)
{
  block_stack.top()->push_back(op);
}

void output_il_context::output_begin_block(il_node::il_operator* op, list<il_node::il_operator*>* bl)
{
  block_stack.top()->push_back(op);
  block_stack.push(bl);
}

void output_il_context::end_block()
{
  block_stack.pop();
}

void output_il_context::swap_result(vector<list<il_node::il_operator*>*>& out)
{
  // ����Ĥ��Ƥ��뤳�Ȥ��ǧ
  assert(block_stack.size() == 0);

  // result�������vector�˰ܤ�
  out.swap(result);
}

