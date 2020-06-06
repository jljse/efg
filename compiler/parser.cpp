#include <string>
#include <iostream>
#include "parser.h"
#include "p_yacc.hh"
#include "p_node.h"

using namespace std;

p_node::p_root* parser::parse(const string& name)
{
  init_yy(name);
  yy::p_yacc p(*this);
  int result = p.parse();
  finit_yy();

  if(result == 0){
    return get_result_yy();
  }else{
    return NULL;
  }
}

void parser::error(const yy::location& loc, string s)
{
  cout << loc << ": " << s << endl;
  exit(1);
}

void parser::error(string s)
{
  cout << s << endl;
  exit(1);
}

void parser::set_result_yy(p_node::p_root* v)
{
  result_yy = v;
}

p_node::p_root* parser::get_result_yy()
{
  return result_yy;
}

