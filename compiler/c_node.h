#ifndef C_NODE_H_INCLUDED
#define C_NODE_H_INCLUDED

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <ostream>
#include <sstream>
#include <set>
#include <cassert>
#include "c_to_il.h"
#include "utility.h"

/*
  プログラムを単純なグラフ構造として表現するためのクラス
  
  c_atomic      : 引数を持つグラフノードの抽象クラス
  c_atom        : atomic 普通のアトム
  c_integer     : atomic 整数リテラル
  c_float       : atomic 小数リテラル
  c_pcontext    : atomic (型つき)プロセス文脈
  c_link        : リンクの1つの端 自分のくっついているatomicと引数番号,反対側の端の情報を持つ
  c_rule        : ルール head,bodyはそれぞれグラフ構造として持つ
*/

namespace c_node
{
  class c_atomic;
  class c_link;
  class c_rule;

  class c_base
  {
  public:
    location loc;
    void set_loc(int line, int column) { loc.line=line; loc.column=column; }
    c_base() { loc.line=-1; loc.column=-1; }
    virtual ~c_base() {}
  };

  class c_root : public c_base
  {
  public:
    // ルール全て
    std::vector<c_rule*> rules;
    // 初期データ構造のアトム全てへのポインタ どこを始点にすればいいか分からない
    std::vector<c_atomic*> atomics;
    // 全部をoに出力
    void output(std::ostream& o) const;
  };
  
  class c_atomic : public c_base
  {
  public:
    std::vector<c_link*> arguments;
    virtual ~c_atomic() {}
    c_atomic(int a) { arguments.resize(a); }
    virtual std::string name_string() const = 0;
    // そのアトムがマッチの始点になれるかどうか
    virtual bool is_compile_head_root() const { return false; }
    // このアトムを始点にしてマッチする命令を出力
    virtual void compile_head_root(matching_context& m, output_il_context& o) const;
    // このアトムを子としてマッチする命令を出力(子孫のマッチも中で行う) index:nの引数から入ってマッチを行う
    virtual void compile_head_child(matching_context& m, output_il_context& o, int n) const;
    // マッチしたアトム1つを削除する命令を出力
    virtual void compile_body_delete(const matching_context& m, output_il_context& o) const;
    // このアトム1つを生成する命令を出力
    virtual void compile_body(matching_context& m, output_il_context& o) const;
  };
  
  class c_atom : public c_atomic
  {
  public:
    std::string name;
    c_atom(std::string n, int a) : c_atomic(a) { name=n; };
    virtual std::string name_string() const { return name; }
    virtual bool is_compile_head_root() const { return true; }
    virtual void compile_head_root(matching_context& m, output_il_context& o) const;
    virtual void compile_head_child(matching_context& m, output_il_context& o, int n) const;
    virtual void compile_body_delete(const matching_context& m, output_il_context& o) const;
    /* virtual void compile_body(matching_context* m, output_il_context* o); */
  };
  
  class c_integer : public c_atomic
  {
  public:
    int data;
    c_integer(int d) : c_atomic(1) { data=d; }
    virtual std::string name_string() const { std::stringstream ss; ss<<data; return ss.str(); }
    virtual void compile_head_child(matching_context& mcontext, output_il_context& ocontext, int pos) const;
    virtual void compile_body_delete(const matching_context& m, output_il_context& o) const;
    /* virtual void compile_body(matching_context* m, output_il_context* o); */
  };
  
  class c_float : public c_atomic
  {
  public:
    double data;
    c_float(double d) : c_atomic(1) { data=d; }
    virtual std::string name_string() const { std::stringstream ss; ss<<data; return ss.str(); }
  };
  
  class c_pcontext : public c_atomic
  {
  public:
    std::string name;
    std::string type;
    c_pcontext(std::string n, std::string t) : c_atomic(1), name(n), type(t) { }
    virtual std::string name_string() const { std::stringstream ss; ss<<name; if(type!=""){ ss<<":"<<type; } return ss.str(); }
    virtual void compile_head_child(matching_context& mcontext, output_il_context& ocontext, int pos) const;
    virtual void compile_body_delete(const matching_context& m, output_il_context& o) const; /* pcontextはheadでは消さないので何もしない関数 */
    /* virtual void compile_body(matching_context* m, output_il_context* o); */
  };
  
  class c_link : public c_base
  {
  public:
    // プログラム中で明示的に名前がつけられている場合のみ. それ以外は空文字列.
    std::string name;
    c_atomic* atomic;
    int pos;
    c_link* buddy;
    // 名前つきリンク端を生成する場合
    c_link(std::string n) : name(n) { atomic=NULL; pos=-1; buddy=NULL; }
    // 名前なしリンク端を生成する場合
    c_link(c_atomic* a, int p) : name("") { atomic=a; pos=p; buddy=NULL; }
    // 別の端と互いに接続する
    void connect(c_link* b) { assert(this!=b); buddy=b; b->buddy=this; }
    // このリンク側をマッチする命令を出力
    void compile_head(matching_context& m, output_il_context& o) const;
    // 自由リンクかどうか ( atom-link-link-atom が自由リンクでは atom-link-link-NULL になっている )
    bool is_freelink() const { return atomic == NULL; }
  };
  
  class c_rule : public c_base
  {
  public:
    // head側の自由リンクとbody側の自由リンクのペア
    // 自由リンクはatomicがnullになっている
    std::vector<std::pair<c_link*,c_link*> > freelinks;
    // head側のpcontextとbody側のpcontextの対応
    std::vector<std::pair<c_pcontext*,std::vector<c_pcontext*> > > pcontexts;
    // head側の全アトム (含むpcontext)
    std::vector<c_atomic*> head;
    // body側の全アトム (含むpcontext)
    std::vector<c_atomic*> body;
    
    void output(std::ostream& o) const;
  };

}

// 同名の自由リンクを接続し,配列から消す.
// 3回以上出てきたリンクがあったらエラー.
// freelinksに残るのは1回のみ出現したリンク
void connect_localfreelinks(std::vector<c_node::c_link*>& freelinks);

// aのアトムの集合をoに出力
// aで閉じていないリンクの情報はlで受け渡す
// あるc_linkに対して使う名前が既に決まっていればlに入れた状態で関数を呼び,
// 関数終了後に自由リンクが残っていれば,その自由リンクと名前の情報をlに返す.
// 次の自由リンクに使えるidをiに与え,iに返す
void output_graph(std::ostream& o,
                  const std::vector<c_node::c_atomic*>& a,
                  std::map<c_node::c_link*,std::string>& l,
                  int& i);

#endif
