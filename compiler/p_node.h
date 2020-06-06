#ifndef P_NODE_H_INCLUDED
#define	P_NODE_H_INCLUDED

#include <string>
#include <vector>
#include <iostream>
#include "utility.h"
#include "c_node.h"
#include "p_to_c.h"

/*
  パージングツリーを生成するためのノード
  あくまでパージングツリーなので実際のグラフ構造とは全く異なる
  
  p_base     :                              || パージングツリーにおけるノードの抽象クラス
  p_graphs   : hoge , hoge , hoge...        || アトムやリンク接続文の並び(.で終わるまで)
  p_assign   : hoge = hoge                  || リンク接続文
  p_atom     : hoge( , , , )                || アトム
  p_rule     : hoge :- hoge                 || ルール
  p_biop     : hoge op hoge                 || 2項演算子
  p_link     : HOGE                         || リンクの名前
  p_integer  : 774                          || 整数リテラル
  p_float    : 7.74                         || 小数リテラル
  p_list     : [ , , , | ]                  || リスト
  p_pcontext : $hoge : hoge                 || 型つきプロセス文脈 (膜がないので普通のプロセス文脈は存在しない)

  パージングツリー内のオブジェクトは文字列ポインタも含め全てデストラクタでdeep deleteされるため
  パージングツリーを消す前に必要な情報はコピーしておくこと
*/

namespace p_node
{
  class p_base
  {
  public:
    // 出現位置
    location loc;

    void set_loc(int line, int column) { loc.line=line, loc.column=column; }
    p_base() { loc.line=-1; loc.column=-1; }
    virtual ~p_base() {}
    // oに自身以下のツリーを出力, levelはツリーの深さ(最初は0で呼ぶ)
    virtual std::ostream& output(std::ostream& o, int level) const = 0;
    // 自身以下のツリーをルートとしてコンパイル. 自由リンク等はcで処理する.
    virtual void to_c_node_as_root(p_to_c_context& c) const;
    // 自身以下のツリーをリンクへの埋め込みの略記としてコンパイル. 自由リンク等はcで処理する.
    virtual c_node::c_link* to_c_node_as_child(p_to_c_context& c) const;
  };

  class p_root : public p_base
  {
  public:
    std::vector<p_base*>* data;
    p_root(std::vector<p_base*>* d) { data=d; }
    ~p_root() { delete_vector(data); }
    virtual std::ostream& output(std::ostream& o, int level) const;
    virtual void to_c_node_as_root(p_to_c_context& c) const;
  };

  class p_graphs : public p_base
  {
  public:
    std::vector<p_base*>* data;
    p_graphs(std::vector<p_base*>* d) { data=d; }
    ~p_graphs() { delete_vector(data); }
    virtual std::ostream& output(std::ostream& o, int level) const;
    virtual void to_c_node_as_root(p_to_c_context& c) const;
  };

  class p_assign : public p_base
  {
  public:
    p_base* lhs;
    p_base* rhs;
    p_assign(p_base* l, p_base* r) { lhs=l; rhs=r; }
    ~p_assign() { delete lhs; delete rhs; }
    virtual std::ostream& output(std::ostream& o, int level) const;
    virtual void to_c_node_as_root(p_to_c_context& c) const;
  };

  class p_atom : public p_base
  {
  public:
    std::string* name;
    std::vector<p_base*>* arguments;
    p_atom(std::string* n, std::vector<p_base*>* a) { name=n; arguments=a; }
    ~p_atom() { delete name; delete_vector(arguments); }
    virtual std::ostream& output(std::ostream& o, int level) const;
    virtual void to_c_node_as_root(p_to_c_context& c) const;
    virtual c_node::c_link* to_c_node_as_child(p_to_c_context& c) const;
  };

  class p_rule : public p_base
  {
  public:
    std::vector<p_base*>* head;
    std::vector<p_base*>* body;
    p_rule(std::vector<p_base*>* h, std::vector<p_base*>* b) { head=h; body=b; }
    ~p_rule() { delete_vector(head); delete_vector(body); }
    virtual std::ostream& output(std::ostream& o, int level) const;
    virtual void to_c_node_as_root(p_to_c_context& c) const;
  };

  class p_biop : public p_base
  {
  public:
    std::string name;
    p_base* lhs;
    p_base* rhs;
    p_biop(const std::string& n, p_base* l, p_base* r) : name(n) { lhs=l; rhs=r; }
    ~p_biop() { delete lhs; delete rhs; }
    virtual std::ostream& output(std::ostream& o, int level) const;
    virtual c_node::c_link* to_c_node_as_child(p_to_c_context& c) const;
  };

  class p_link : public p_base
  {
  public:
    std::string* name;
    p_link(std::string* n) { name=n; }
    ~p_link() { delete name; };
    virtual std::ostream& output(std::ostream& o, int level) const;
    virtual c_node::c_link* to_c_node_as_child(p_to_c_context& c) const;
  };

  class p_integer : public p_base
  {
  public:
    int data;
    p_integer(int d) { data=d; }
    virtual std::ostream& output(std::ostream& o, int level) const;
    virtual c_node::c_link* to_c_node_as_child(p_to_c_context& c) const;
  };

  class p_float : public p_base
  {
  public:
    double data;
    p_float(double d) { data=d; }
    virtual std::ostream& output(std::ostream& o, int level) const;
    virtual c_node::c_link* to_c_node_as_child(p_to_c_context& c) const;
  };

  class p_string : public p_base
  {
  public:
    std::string* data;
    p_string(std::string* d) { data=d; }
    ~p_string() { delete data; }
    virtual std::ostream& output(std::ostream& o, int level) const;
    virtual c_node::c_link* to_c_node_as_child(p_to_c_context& c) const;
  };

  class p_list : public p_base
  {
  public:
    std::vector<p_base*>* arguments;
    p_base* rest;
    p_list(std::vector<p_base*>* a, p_base* r) { arguments=a; rest=r; }
    ~p_list() { delete_vector(arguments); delete rest; }
    virtual std::ostream& output(std::ostream& o, int level) const;
    virtual c_node::c_link* to_c_node_as_child(p_to_c_context& c) const;
  };

  class p_pcontext : public p_base
  {
  public:
    std::string* name;
    std::string* type;
    p_pcontext(std::string* n, std::string* t) { name=n; type=t; }
    ~p_pcontext() { delete name; delete type; }
    virtual std::ostream& output(std::ostream& o, int level) const;
    virtual c_node::c_link* to_c_node_as_child(p_to_c_context& c) const;
  };

}

#endif
