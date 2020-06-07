#include <iostream>
#include <map>
#include <algorithm>
#include <iterator>
#include "p_node.h"
#include "utility.h"

using namespace p_node;
using std::map;
using std::string;
using std::vector;
using std::ostream;

void p_base::to_c_node_as_root(p_to_c_context& c) const
{
  std::cerr << "unexpected node as root at " << loc << "." << std::endl;
  exit(1);
}

c_node::c_link* p_base::to_c_node_as_child(p_to_c_context& c) const
{
  std::cerr << "unexpected node as child at " << loc << "." << std::endl;
  exit(1);
}

ostream& p_root::output(ostream& o, int level) const
{
  o << ntimes_space(level) << "root" << loc << std::endl;
  for(vector<p_base*>::iterator it=data->begin(); it!=data->end(); ++it){
    p_base* x = *it;
    x->output(o, level+1);
  }
  return o;
}

void p_root::to_c_node_as_root(p_to_c_context& c) const
{
  for(unsigned int i=0; i<data->size(); ++i){
    (*data)[i]->to_c_node_as_root(c);
  }
}

ostream& p_graphs::output(ostream& o, int level) const
{
  o << ntimes_space(level) << "graphs" << loc << std::endl;
  for(vector<p_base*>::iterator it=data->begin(); it!=data->end(); ++it){
    (*it)->output(o, level+1);
  }
  return o;
}

void p_graphs::to_c_node_as_root(p_to_c_context& c) const
{
  p_to_c_context statement_scope;
  for(unsigned int i=0; i<data->size(); ++i){
    (*data)[i]->to_c_node_as_root(statement_scope);
  }
  
  // 自由リンク同士を接続する
  connect_localfreelinks(statement_scope.freelinks);
  if(statement_scope.freelinks.size() > 0){
    for(unsigned int i=0; i<statement_scope.freelinks.size(); ++i){
      std::cerr << "there is unconnected link \""
                << statement_scope.freelinks[i]->name
                << "\" at "
                << statement_scope.freelinks[i]->loc
                << "."
                << std::endl;
    }
    exit(1);
  }

  // アトムを全部集める
  std::copy(statement_scope.atomics.begin(),
            statement_scope.atomics.end(),
            std::back_inserter(c.atomics));
}

ostream& p_assign::output(ostream& o, int level) const
{
  o << ntimes_space(level) << "assign" << loc << std::endl;
  lhs->output(o, level+1);
  rhs->output(o, level+1);
  return o;
}

void p_assign::to_c_node_as_root(p_to_c_context& c) const
{
  c_node::c_link* l = lhs->to_c_node_as_child(c);
  c_node::c_link* r = rhs->to_c_node_as_child(c);
  l->connect(r);
}

ostream& p_atom::output(ostream& o, int level) const
{
  o << ntimes_space(level) << "atom"  << loc << " : \"" << *name << "\"" << std::endl;
  for(vector<p_base*>::iterator it=arguments->begin(); it!=arguments->end(); ++it){
    (*it)->output(o, level+1);
  }
  return o;
}

void p_atom::to_c_node_as_root(p_to_c_context& c) const
{
  c_node::c_atom* a = new c_node::c_atom(*name, arguments->size());
  a->set_loc(loc.line, loc.column);
  c.regist_atomic(a);
  for(unsigned int i=0; i<arguments->size(); ++i){
    c_node::c_link* x = new c_node::c_link(a, i);
    a->arguments[i] = x;
    c_node::c_link* y = (*arguments)[i]->to_c_node_as_child(c);
    x->connect(y);
  }
}

c_node::c_link* p_atom::to_c_node_as_child(p_to_c_context& c) const
{
  c_node::c_atom* a = new c_node::c_atom(*name, arguments->size() + 1);
  a->set_loc(loc.line, loc.column);
  c.regist_atomic(a);
  for(unsigned int i=0; i<arguments->size(); ++i){
    c_node::c_link* x = new c_node::c_link(a, i);
    a->arguments[i] = x;
    c_node::c_link* y = (*arguments)[i]->to_c_node_as_child(c);
    x->connect(y);
  }

  c_node::c_link* result = new c_node::c_link(a, a->arguments.size() - 1);
  a->arguments[a->arguments.size()-1] = result;
  return result;
}

ostream& p_rule::output(ostream& o, int level) const
{
  o << ntimes_space(level) << "rule" << loc << std::endl;
  for(vector<p_base*>::iterator it=head->begin(); it!=head->end(); ++it){
    (*it)->output(o, level+1);
  }
  o << ntimes_space(level) << ":-" << std::endl;
  for(vector<p_base*>::iterator it=body->begin(); it!=body->end(); ++it){
    (*it)->output(o, level+1);
  }
  return o;
}

void p_rule::to_c_node_as_root(p_to_c_context& c) const
{
  c_node::c_rule* rule = new c_node::c_rule;
  rule->set_loc(loc.line, loc.column);
  c.regist_rule(rule);
  
  // ヘッド側をコンパイル, ヘッド内で完結したリンクを解決
  p_to_c_context head_scope;
  for(unsigned int i=0; i<head->size(); ++i){
    (*head)[i]->to_c_node_as_root(head_scope);
  }
  connect_localfreelinks(head_scope.freelinks);
  rule->head = head_scope.atomics;
  
  // ボディ側
  p_to_c_context body_scope;
  for(unsigned int i=0; i<body->size(); ++i){
    (*body)[i]->to_c_node_as_root(body_scope);
  }
  connect_localfreelinks(body_scope.freelinks);
  rule->body = body_scope.atomics;
  
  // ヘッド側の自由リンクとボディ側の自由リンクをペアにする
  for(vector<c_node::c_link*>::iterator it=head_scope.freelinks.begin();
      it!=head_scope.freelinks.end();
      ++it)
  {
    for(vector<c_node::c_link*>::iterator jt=body_scope.freelinks.begin();
        jt!=body_scope.freelinks.end();
        ++jt)
    {
      if(*jt && (*it)->name == (*jt)->name){ //*jtは既に消されている場合があるので注意
        rule->freelinks.push_back(std::make_pair(*it, *jt));
        *it = NULL;
        *jt = NULL;
        break;
      }
    }
  }
  head_scope.freelinks.erase(std::remove(head_scope.freelinks.begin(), head_scope.freelinks.end(), static_cast<c_node::c_link*>(NULL)), head_scope.freelinks.end());
  body_scope.freelinks.erase(std::remove(body_scope.freelinks.begin(), body_scope.freelinks.end(), static_cast<c_node::c_link*>(NULL)), body_scope.freelinks.end());
  
  // ここでheadやbodyにfreelinksとしてリンクが残っているなら,接続失敗
  if(head_scope.freelinks.size()>0 || body_scope.freelinks.size()>0){
    vector<c_node::c_link*> unconnected_link = head_scope.freelinks;
    std::copy(body_scope.freelinks.begin(),
              body_scope.freelinks.end(),
              std::back_inserter(unconnected_link));
    for(vector<c_node::c_link*>::iterator it=unconnected_link.begin();
        it!=unconnected_link.end();
        ++it)
    {
      std::cerr << " there is unconnected link \""
                << (*it)->name
                << "\" at "
                << (*it)->loc
                << "."
                << std::endl;
    }
    exit(1);
  }

  // ヘッド側のプロセス文脈とボディ側のプロセス文脈の対応を確認する
  typedef std::map<std::string,c_node::c_pcontext*> pcontext_head_mapping_type;
  pcontext_head_mapping_type pcontext_head_mapping;
  typedef std::map<std::string,std::vector<c_node::c_pcontext*> > pcontext_body_mapping_type;
  pcontext_body_mapping_type pcontext_body_mapping;
  bool error_flag = false;
  
  // ヘッド側の情報を確認
  for(unsigned int i=0; i<head_scope.pcontexts.size(); ++i){
    c_node::c_pcontext* it = head_scope.pcontexts[i];
    if(it->type == ""){
      // 型の指定がない場合エラー
      error_flag = true;
      std::cerr << "there is untyped process context \""
                << it->name
                << "\" at "
                << it->loc
                << "."
                << std::endl;
    }
    if(it->type == "int"){
      // 整数型 OK
    } else {
      // それ以外はダメ
      error_flag = true;
      std::cerr << "unknwon type process context \""
                << it->name << ":" << it->type
                << "\" at "
                << it->loc
                << "."
                << std::endl;
    }
    if(pcontext_head_mapping.find(it->name) != pcontext_head_mapping.end()){
      // 同じ名前をヘッドで2回使っている場合エラー
      error_flag = true;
      std::cerr << "there is name collision of process context \""
                << it->name
                << "\" at "
                << it->loc
                << "."
                << std::endl;
    }
    // 初出のプロセス文脈を登録
    pcontext_head_mapping.insert(std::make_pair(it->name, it));
  }
  if(error_flag) exit(1);
  
  // ボディ側の情報を確認
  for(unsigned int i=0; i<body_scope.pcontexts.size(); ++i){
    c_node::c_pcontext* it = body_scope.pcontexts[i];
    if(it->type != ""){
      error_flag = true;
      std::cerr << "there is type declaration in body process context \""
                << it->name
                << "\" at "
                << it->loc
                << "."
                << std::endl;
      // 型の指定がついている場合エラー (ボディ側で型指定はしない)
    }
    if(pcontext_head_mapping.find(it->name) == pcontext_head_mapping.end()){
      // その名前がヘッド側に出てきていない場合エラー
      error_flag = true;
      std::cerr << "there is unknown process context \""
                << it->name
                << "\" at "
                << it->loc
                << "."
                << std::endl;
    }
    // プロセス文脈のボディでの出現を登録
    pcontext_body_mapping[it->name].push_back(it);
  }
  if(error_flag) exit(1);
  
  // プロセス文脈のヘッドとボディの対応を記録しておく
  // マッチング中の情報が欲しいこと、ボディ生成の順番の問題があることから
  // rule->headにもbodyにもpcontextを入れてあるが、別に対応があった方がわかりやすい
  for(unsigned int i=0; i<head_scope.pcontexts.size(); ++i){
    c_node::c_pcontext* it = head_scope.pcontexts[i];
    rule->pcontexts.push_back(make_pair(it,
                                        pcontext_body_mapping[it->name]));
  }
  
  // TODO: マッチした後ボディ側ではどういうデータで引き回すのか？ (intアトムなのかintなのか)
  // TODO: 整数操作はインラインでやりたい いつ整数アトムを作るのか？ 最適化でやるの？
  // プロセス文脈のコピー等はアトムとは別扱いで個別に命令を発行する deletepc/copypc? それとも deleteint/copyint?
}

ostream& p_biop::output(ostream& o, int level) const
{
  o << ntimes_space(level) << "biop" << loc << " : \"" << name << "\"" << std::endl;
  lhs->output(o, level+1);
  rhs->output(o, level+1);
  return o;
}

c_node::c_link* p_biop::to_c_node_as_child(p_to_c_context& c) const
{
  c_node::c_atom* a = new c_node::c_atom(name, 3);
  a->set_loc(loc.line, loc.column);
  c.regist_atomic(a);
  
  c_node::c_link* l = new c_node::c_link(a, 0);
  a->arguments[0] = l;
  c_node::c_link* p = lhs->to_c_node_as_child(c);
  p->connect(l);
  
  c_node::c_link* r = new c_node::c_link(a, 1);
  a->arguments[1] = r;
  c_node::c_link* q = rhs->to_c_node_as_child(c);
  q->connect(r);
  
  c_node::c_link* result = new c_node::c_link(a, 2);
  a->arguments[2] = result;
  return result;
}

ostream& p_link::output(ostream& o, int level) const
{
  o << ntimes_space(level) << "link" << loc << " : \"" << *name << "\"" << std::endl;
  return o;
}

c_node::c_link* p_link::to_c_node_as_child(p_to_c_context& c) const
{
  c_node::c_link* l = new c_node::c_link(*name);
  l->set_loc(loc.line, loc.column);
  c.regist_freelink(l);
  return l;
}

ostream& p_integer::output(ostream& o, int level) const
{
  o << ntimes_space(level) << "int" << loc << " : \"" << data << "\"" << std::endl;
  return o;
}

c_node::c_link* p_integer::to_c_node_as_child(p_to_c_context& c) const
{
  c_node::c_integer* a = new c_node::c_integer(data);
  a->set_loc(loc.line, loc.column);
  c.regist_atomic(a);
  c_node::c_link* result = new c_node::c_link(a, 0);
  a->arguments[0] = result;
  return result;
}

ostream& p_float::output(ostream& o, int level) const
{
  o << ntimes_space(level) << "float" << loc << " : \"" << data << "\"" << std::endl;
  return o;
}

c_node::c_link* p_float::to_c_node_as_child(p_to_c_context& c) const
{
  c_node::c_float* a = new c_node::c_float(data);
  a->set_loc(loc.line, loc.column);
  c.regist_atomic(a);
  c_node::c_link* result = new c_node::c_link(a, 0);
  a->arguments[0] = result;
  return result;
}

ostream& p_string::output(ostream& o, int level) const
{
  o << ntimes_space(level) << "string"  << loc << " : \"" << *data << "\"" << std::endl;
  return o;
}

c_node::c_link* p_string::to_c_node_as_child(p_to_c_context& c) const
{
  // 文字列は文字コードの整数をリストにしたものとする
  c_node::c_atom* nil = new c_node::c_atom("[]", 1);
  c.regist_atomic(nil);
  c_node::c_link* head = new c_node::c_link(nil, 0);
  nil->arguments[0] = head;
  
  for(int i=data->size()-1; i>=0; --i){
    c_node::c_atom* dot = new c_node::c_atom(".", 3);
    c.regist_atomic(dot);
    c_node::c_link* dot_0 = new c_node::c_link(dot, 0);
    dot->arguments[0] = dot_0;
    c_node::c_link* dot_1 = new c_node::c_link(dot, 1);
    dot->arguments[1] = dot_1;
    c_node::c_link* dot_2 = new c_node::c_link(dot, 2);
    dot->arguments[2] = dot_2;
    
    c_node::c_integer* x = new c_node::c_integer((*data)[i]);
    c.regist_atomic(x);
    c_node::c_link* l = new c_node::c_link(x, 0);
    x->arguments[0] = l;
    
    l->connect(dot_0);
    head->connect(dot_1);
    head = dot_2;
  }
  
  return head;
}

ostream& p_list::output(ostream& o, int level) const
{
  o << ntimes_space(level) << "list" << loc << std::endl;
  for(vector<p_base*>::iterator it=arguments->begin(); it!=arguments->end(); ++it){
    (*it)->output(o, level+1);
  }
  if(rest){
    o << ntimes_space(level) << "|" << std::endl;
    rest->output(o, level+1);
  }
  return o;
}

c_node::c_link* p_list::to_c_node_as_child(p_to_c_context& c) const
{
  c_node::c_link* head;
  if(rest){
    head = rest->to_c_node_as_child(c);
  }else{
    c_node::c_atom* nil = new c_node::c_atom("[]", 1);
    c.regist_atomic(nil);
    c_node::c_link* l = new c_node::c_link(nil, 0);
    nil->arguments[0] = l;
    head = l;
  }
  
  for(int i=arguments->size()-1; i>=0; --i){
    c_node::c_atom* dot = new c_node::c_atom(".", 3);
    c.regist_atomic(dot);
    c_node::c_link* dot_0 = new c_node::c_link(dot, 0);
    dot->arguments[0] = dot_0;
    c_node::c_link* dot_1 = new c_node::c_link(dot, 1);
    dot->arguments[1] = dot_1;
    c_node::c_link* dot_2 = new c_node::c_link(dot, 2);
    dot->arguments[2] = dot_2;
    
    c_node::c_link* l = (*arguments)[i]->to_c_node_as_child(c);
    l->connect(dot_0);
    head->connect(dot_1);
    head = dot_2;
  }
  
  return head;
}

ostream& p_pcontext::output(ostream& o, int level) const
{
  o << ntimes_space(level) << "pcontext"  << loc << " : " << *name << " : " << *type << std::endl;
  return o;
}

c_node::c_link* p_pcontext::to_c_node_as_child(p_to_c_context& c) const
{
  c_node::c_pcontext* p = new c_node::c_pcontext(*name, *type);
  p->set_loc(loc.line, loc.column);
  c.regist_pcontext(p);
  c.regist_atomic(p);
  
  c_node::c_link* l = new c_node::c_link(p, 0);
  p->arguments[0] = l;
  return l;
}
