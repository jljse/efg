#ifndef C_TO_IL_H_INCLUDED
#define C_TO_IL_H_INCLUDED

#include <map>
#include <vector>
#include <stack>
#include <list>
#include <set>
#include "il_node.h"

// 循環include回避
namespace c_node
{
  class c_atomic;
  class c_link;
};

// ルールのマッチング生成状況
class matching_context
{
public:
  // マッチ/生成済みのアトム(map:アトムオブジェクト->そのindex)
  typedef std::map<const c_node::c_atomic*,int> found_atom_type;
  found_atom_type found_atom;
  // マッチ/生成済みのリンク(map:リンクオブジェクト->そのindex)
  typedef std::map<const c_node::c_link*,int> found_link_type;
  found_link_type found_link;
  // isbuddyでループをチェック済みのリンク
  std::set<const c_node::c_link*> isbuddy_checked;
  
  int next_index;

  matching_context() { next_index = 0; } // 0番は主導アトム
  // 各マッチング対象がマッチ/生成していればそれのあるindexを返し, マッチ/生成していなければ-1を返す
  int get_index(const c_node::c_atomic* x) const;
  int get_index(const c_node::c_link* x) const;

  // 各対象にindexをふる 初出であることを期待 indexを返す
  int add_index(const c_node::c_atomic* x);
  int add_index(const c_node::c_link* x);
  // 適当にスキップ
  int add_index();

  // ループチェックが終了しているかどうか
  bool is_isbuddy_checked(const c_node::c_link* x);
  // ループチェック済みを記録
  void add_isbuddy_checked(const c_node::c_link* x);
};

// 出力するilの生成状況
class output_il_context
{
public:
  // 出力したルール
  std::vector<std::list<il_node::il_operator*>*> result;
  // 出力先ブロック
  std::stack<std::list<il_node::il_operator*>*> block_stack;
  
  // ルール出力を開始する
  void begin_rule();
  // 出力中のルールを終了する
  void end_rule();
  // 今出力中のブロックに命令を出力
  void output(il_node::il_operator*);
  // 今出力中のブロックに命令を出力した後,出力先のブロックを変更する
  void output_begin_block(il_node::il_operator*, std::list<il_node::il_operator*>*);
  // 今出力中のブロックを閉じ,それ以前のブロックに出力先を変更する
  void end_block();
  // 出力結果を得る (引数に与えたvectorにswapで結果を返す)
  void swap_result(std::vector<std::list<il_node::il_operator*>*>&);
};

#include "c_node.h"

#endif
