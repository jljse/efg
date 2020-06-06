#ifndef IL_NODE_H_INCLUDED
#define IL_NODE_H_INCLUDED

#include <list>
#include <vector>
#include <string>
#include <ostream>

namespace il_node
{
  
  // 中間命令内で使われる引数
  class arg_base
  {
  public:
    virtual ~arg_base() {};
    virtual void output(std::ostream& o, int indent) const = 0;
    // 浅い比較(入れ子blockの中までは見ない)
    virtual bool is_same_shallow(const arg_base*) const { return true; }
  };

  // 中間命令1つ
  class il_operator
  {
  public:
    std::string opecode;
    std::vector<arg_base*> arguments;
    void output(std::ostream& o, int indent) const;
    il_operator(const std::string&);
    il_operator(const std::string&, arg_base*);
    il_operator(const std::string&, arg_base*, arg_base*);
    il_operator(const std::string&, arg_base*, arg_base*, arg_base*);
    il_operator(const std::string&, arg_base*, arg_base*, arg_base*, arg_base*);
    // 命令が消えるとき,持っている引数もdeleteする
    ~il_operator();
    // まったく同じ命令かどうかを返す 入れ子のブロックは見ない
    bool is_same_shallow(const il_operator* x) const;
  };

  // 中間命令全体
  class il_root
  {
  public:
    // ルール
    std::vector<std::list<il_operator*>*> rules;
    // 初期データ
    std::list<il_operator*> initial;
    void output(std::ostream& o) const;
  };

  // 中間変数のインデックスを表す引数
  class arg_index : public arg_base
  {
  public:
    int index;
    virtual void output(std::ostream& o, int indent) const { o<<"@"<<index; }
    virtual bool is_same_shallow(const arg_base*) const;
    arg_index(int i) { index=i; }
  };

  // 整数の即値を表す引数
  class arg_integer : public arg_base
  {
  public:
    int data;
    virtual void output(std::ostream& o, int indent) const { o<<"#"<<data; }
    virtual bool is_same_shallow(const arg_base*) const;
    arg_integer(int i) { data=i; }
  };

  // ファンクタを表す引数
  class arg_functor : public arg_base
  {
  public:
    std::string name;
    int arity;
    virtual void output(std::ostream& o, int indent) const { o<<"$"<<name<<":"<<arity; }
    virtual bool is_same_shallow(const arg_base*) const;
    arg_functor(std::string n, int a) : name(n), arity(a) { }
  };

  // プロセス文脈を表す引数
  class arg_pcontext : public arg_base
  {
  public:
    std::string type;
    virtual void output(std::ostream& o, int indent) const { o<<"?"<<type; }
    virtual bool is_same_shallow(const arg_base*) const;
    arg_pcontext(std::string t) : type(t) {}
  };

  // 入れ子状の中間命令を表す引数
  class arg_block : public arg_base
  {
  public:
    std::list<il_operator*> block;
    virtual void output(std::ostream& o, int indent) const;
    virtual ~arg_block();
  };

  // 不定長引数を表す引数
  class arg_array : public arg_base
  {
  public:
    std::vector<arg_base*> array;
    virtual bool is_same_shallow(const arg_base*) const;
    virtual void output(std::ostream& o, int indent) const;
    virtual ~arg_array();
  };

  // ワイルドカード 何が相手でもis_same_shallowがtrueになる
  class arg_wildcard : public arg_base
  {
  public:
    virtual bool is_same_shallow(const arg_base* x) const { return true; }
    virtual void output(std::ostream& o, int indent) const { o<<"*wildcard*"; }
    virtual ~arg_wildcard() { }
  };

}

#endif
