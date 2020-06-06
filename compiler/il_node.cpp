#include "il_node.h"
#include "utility.h"
#include <algorithm>

void il_node::il_operator::output(std::ostream& o, int indent) const
{
  o << ntimes_space(indent)
    << opecode
    << " [";

  for(unsigned int i=0; i<arguments.size(); ++i){
    if(i != 0) o << ",";
    arguments[i]->output(o, indent + 1);
  }

  o << "]" << std::endl;
}

il_node::il_operator::il_operator(const std::string& op)
{
  opecode = op;
}

il_node::il_operator::il_operator(const std::string& op,
                                  il_node::arg_base* a0)
{
  opecode = op;
  arguments.push_back(a0);
}

il_node::il_operator::il_operator(const std::string& op,
                                  il_node::arg_base* a0,
                                  il_node::arg_base* a1)
{
  opecode = op;
  arguments.push_back(a0);
  arguments.push_back(a1);
}

il_node::il_operator::il_operator(const std::string& op,
                                  il_node::arg_base* a0,
                                  il_node::arg_base* a1,
                                  il_node::arg_base* a2)
{
  opecode = op;
  arguments.push_back(a0);
  arguments.push_back(a1);
  arguments.push_back(a2);
}

il_node::il_operator::il_operator(const std::string& op,
                                  il_node::arg_base* a0,
                                  il_node::arg_base* a1,
                                  il_node::arg_base* a2,
                                  il_node::arg_base* a3)
{
  opecode = op;
  arguments.push_back(a0);
  arguments.push_back(a1);
  arguments.push_back(a2);
  arguments.push_back(a3);
}

il_node::il_operator::~il_operator()
{
  std::for_each(arguments.begin(), arguments.end(), deleter());
}

bool il_node::il_operator::is_same_shallow(const il_node::il_operator* x) const
{
  if(arguments.size() == x->arguments.size()){
    if(opecode == x->opecode){
      for(unsigned int i=0; i<arguments.size(); ++i){
        // 各引数について浅い比較を行う
        if(! arguments[i]->is_same_shallow(x->arguments[i])){
          return false;
        }
      }
      // 無事に全引数一致したら等しい
      return true;
    }
  }
  return false;
}

void il_node::il_root::output(std::ostream& o) const
{
  for(unsigned int i=0; i<rules.size(); ++i){
    o << "%rule" << std::endl;
    for(std::list<il_node::il_operator*>::const_iterator jt=rules[i]->begin();
        jt!=rules[i]->end();
        ++jt){
      (*jt)->output(o, 1);
    }
  }

  o << "%initial" << std::endl;
  for(std::list<il_node::il_operator*>::const_iterator it=initial.begin();
      it!=initial.end();
      ++it){
    (*it)->output(o, 1);
  }
}

void il_node::arg_block::output(std::ostream& o, int indent) const
{
  o << "{" << std::endl;
  
  for(std::list<il_node::il_operator*>::const_iterator it=block.begin();
      it!=block.end();
      ++it){
    (*it)->output(o, indent + 1);
  }

  o << ntimes_space(indent) << "}";
}

void il_node::arg_array::output(std::ostream& o, int indent) const
{
  o << "(";
  
  for(unsigned int i=0; i<array.size(); ++i){
    if(i != 0) o << ",";
    array[i]->output(o, indent + 1);
  }

  o << ")";
}

bool il_node::arg_index::is_same_shallow(const il_node::arg_base* x) const
{
  if(const il_node::arg_index* y = dynamic_cast<const arg_index*>(x)){
    return y->index == index;
  }else{
    return false;
  }
}

bool il_node::arg_integer::is_same_shallow(const il_node::arg_base* x) const
{
  if(const il_node::arg_integer* y = dynamic_cast<const arg_integer*>(x)){
    return y->data == data;
  }else{
    return false;
  }
}

bool il_node::arg_functor::is_same_shallow(const il_node::arg_base* x) const
{
  if(const il_node::arg_functor* y = dynamic_cast<const arg_functor*>(x)){
    return y->arity==arity && y->name==name;
  }else{
    return false;
  }
}

bool il_node::arg_pcontext::is_same_shallow(const il_node::arg_base* x) const
{
  if(const il_node::arg_pcontext* y = dynamic_cast<const arg_pcontext*>(x)){
    return y->type==type;
  }else{
    return false;
  }
}

il_node::arg_block::~arg_block()
{
  std::for_each(block.begin(), block.end(), deleter());
}

il_node::arg_array::~arg_array()
{
  std::for_each(array.begin(), array.end(), deleter());
}

bool il_node::arg_array::is_same_shallow(const il_node::arg_base* x) const
{
  if(const il_node::arg_array* y = dynamic_cast<const arg_array*>(x)){
    if(y->array.size() == array.size()){
      for(unsigned int i=0; i<array.size(); ++i){
        if(! array[i]->is_same_shallow(y->array[i])){
          // 一致しない引数があったらfalse
          return false;
        }
      }
      return true;
    }
  }
  
  return false;
}

