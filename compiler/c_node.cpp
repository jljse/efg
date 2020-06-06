#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include "c_node.h"
#include "c_to_il.h"

// output_graphに渡す自由リンクと名前の対応. よく使うので名前をつけておく
typedef std::map<c_node::c_link*,std::string> linkname_map;

void c_node::c_root::output(std::ostream& o) const
{
  for(unsigned int i=0; i<rules.size(); ++i){
    rules[i]->output(o);
    o << std::endl;
  }
  
  int linknum = 0;
  linkname_map l;
  output_graph(o, atomics, l, linknum);
  o << "." << std::endl;
  
  for(linkname_map::iterator it=l.begin(); it!=l.end(); ++it){
    std::cerr << "implementation error, there is freelink (" << it->second << ") in main graph." << std::endl;
    exit(1);
  }
}

void c_node::c_rule::output(std::ostream& o) const
{
  int linknum = 0;
  // ヘッド側のグラフを出力し,自由リンクの情報をhead_freelinksに,次のリンク番号の情報をlinknumに受ける
  linkname_map head_freelinks;
  output_graph(o, head, head_freelinks, linknum);
  
  o << " :- ";
  
  // ヘッド側の自由リンクに対応するボディ側の自由リンクの情報を構築する
  linkname_map body_freelinks;
  for(linkname_map::iterator it=head_freelinks.begin();
      it!=head_freelinks.end();
      ++it)
  {
    {
      bool continue_flag = false;
      for(unsigned int i=0; i<freelinks.size(); ++i){
        if(freelinks[i].first == it->first->buddy){ // ヘッド側自由リンクの反対側が登録されているので注意
          body_freelinks[freelinks[i].second] = it->second;
          continue_flag = true;
          break;
        }
      }
      if(continue_flag) continue;
    }
    std::cerr << "implementation error, there is unconnected freelink (" << it->second << ") when printing rule." << std::endl;
  }
  
  // ボディ側のグラフを出力する
  output_graph(o, body, body_freelinks, linknum);
  o << ".";
}



void c_node::c_atomic::compile_head_root(matching_context& m, output_il_context& o) const
{
  std::cerr << "implementation error, unexpected root matching with " << name_string() << " at " << loc << std::endl;
  exit(1);
}

void c_node::c_atomic::compile_head_child(matching_context& m, output_il_context& o, int pos) const
{
  std::cerr << "implementation error, unexpected child matching with " << name_string() << " at " << loc << std::endl;
  exit(1);
}

void c_node::c_atomic::compile_body_delete(const matching_context& m, output_il_context& o) const
{
  std::cerr << "implementation error, unexpected head delete with " << name_string() << " at " << loc << std::endl;
  exit(1);
}

void c_node::c_atomic::compile_body(matching_context& m, output_il_context& o) const
{
  std::cerr << "implementation error, unexpected body generating with " << name_string() << " at " << loc << std::endl;
  exit(1);
}

// このアトムを始点として結合グラフをマッチする命令を出力
void c_node::c_atom::compile_head_root(matching_context& mcontext, output_il_context& ocontext) const
{
  // このアトムに番号をつける
  mcontext.add_index(this);

  // OUTPUT: [findatom]
  il_node::arg_block* block = new il_node::arg_block;
  ocontext.output_begin_block(new il_node::il_operator("p_findatom",
                                                       new il_node::arg_index(mcontext.get_index(this)),
                                                       new il_node::arg_functor(this->name, this->arguments.size()),
                                                       block),
                              &(block->block));

  // マッチ済みの同じ種類のアトムに対してneqatom
  for(matching_context::found_atom_type::const_iterator it=mcontext.found_atom.begin();
      it!=mcontext.found_atom.end();
      ++it){
    if(it->first == static_cast<const c_node::c_atomic*>(this)) continue; // 自分自身は飛ばす (c_atomicとしてのポインタで比較)
    if(const c_node::c_atom* x = dynamic_cast<const c_node::c_atom*>(it->first)){
      if(this->arguments.size()==x->arguments.size() && this->name==x->name){
        // 同じファンクタのアトムを既にマッチしているので, 今findatomしたものが別のアトムであることを確認する必要がある
        // OUTPUT: [neqatom]
        ocontext.output(new il_node::il_operator("p_neqatom",
                                                 new il_node::arg_index(mcontext.get_index(this)),
                                                 new il_node::arg_index(it->second)));
      }
    }
  }

  // 各引数をgetlink
  for(unsigned int i=0; i<this->arguments.size(); ++i){
    // linkに対して番号を与える
    mcontext.add_index(this->arguments[i]);

    // OUTPUT: [getlink]
    ocontext.output(new il_node::il_operator("getlink",
                                             new il_node::arg_index(mcontext.get_index(this->arguments[i])),
                                             new il_node::arg_index(mcontext.get_index(this)),
                                             new il_node::arg_integer(i)));
  }
  
  // 各引数のbuddyに対しマッチを行う
  for(unsigned int i=0; i<this->arguments.size(); ++i){
    this->arguments[i]->buddy->compile_head(mcontext, ocontext);
  }
}

void c_node::c_atom::compile_head_child(matching_context& mcontext, output_il_context& ocontext, int pos) const
{
  // マッチ済みの同じ種類のアトムの、同じ箇所の、未探索な引数に入っちゃったかもしれないのでnotbuddyを出す
  for(matching_context::found_atom_type::const_iterator it=mcontext.found_atom.begin();
      it!=mcontext.found_atom.end();
      ++it){
    if(it->first == static_cast<const c_node::c_atomic*>(this)) continue; // 自分自身は飛ばす (c_atomicとしてのポインタで比較)
    if(const c_node::c_atom* x = dynamic_cast<const c_node::c_atom*>(it->first)){
      if(this->arguments.size()==x->arguments.size() && this->name==x->name){
	if(mcontext.get_index(x->arguments[pos]->buddy) == -1){
	  // OUTPUT: [notbuddy]
	  ocontext.output(new il_node::il_operator("p_notbuddy",
						   new il_node::arg_index(mcontext.get_index(this->arguments[pos])),
						   new il_node::arg_index(mcontext.get_index(x->arguments[pos]))));
	}
      }
    }
  }

  // アトムに番号をふる
  mcontext.add_index(this);
  
  //OUTPUT: [deref]
  ocontext.output(new il_node::il_operator("p_deref",
					   new il_node::arg_index(mcontext.get_index(this)),
					   new il_node::arg_index(mcontext.get_index(this->arguments[pos])),
					   new il_node::arg_integer(pos),
					   new il_node::arg_functor(this->name, this->arguments.size())));

  // 各引数をgetlink
  for(unsigned int i=0; i<this->arguments.size(); ++i){
    // 今来た引数は飛ばす
    if(i == static_cast<unsigned int>(pos)) continue;
    
    // linkに対して番号を与える
    mcontext.add_index(this->arguments[i]);

    // OUTPUT: [getlink]
    ocontext.output(new il_node::il_operator("getlink",
                                             new il_node::arg_index(mcontext.get_index(this->arguments[i])),
                                             new il_node::arg_index(mcontext.get_index(this)),
                                             new il_node::arg_integer(i)));
  }
  
  // 各引数のbuddyに対しcompile_head
  for(unsigned int i=0; i<this->arguments.size(); ++i){
    // 今来た引数は飛ばす
    if(i == static_cast<unsigned int>(pos)) continue;
    
    this->arguments[i]->buddy->compile_head(mcontext, ocontext);
  }
}

void c_node::c_atom::compile_body_delete(const matching_context& mcontext, output_il_context& ocontext) const
{
  // OUTPUT: [freeatom]
  ocontext.output(new il_node::il_operator("freeatom", new il_node::arg_index(mcontext.get_index(this))));
}

// void c_node::c_atom::compile_body(matching_context* m, output_il_context* o)
// {
//   // これはアトム1つだけが対象なの？
//   // call
// }

void c_node::c_integer::compile_head_child(matching_context& mcontext, output_il_context& ocontext, int pos) const
{
  // アトムに番号をふる
  mcontext.add_index(this);
  
  // 整数をマッチ
  ocontext.output(new il_node::il_operator("p_derefint",
					   new il_node::arg_index(mcontext.get_index(this)),
					   new il_node::arg_index(mcontext.get_index(this->arguments[pos])),
					   new il_node::arg_integer(this->data)));
}

void c_node::c_integer::compile_body_delete(const matching_context& mcontext, output_il_context& ocontext) const
{
  // OUTPUT: [freeint]
  ocontext.output(new il_node::il_operator("freeint", new il_node::arg_index(mcontext.get_index(this))));
}

void c_node::c_pcontext::compile_head_child(matching_context& mcontext, output_il_context& ocontext, int pos) const
{
  // アトムに番号をふる
  mcontext.add_index(this);
  
  // プロセス文脈を取得
  ocontext.output(new il_node::il_operator("p_derefpc",
					   new il_node::arg_index(mcontext.get_index(this)),
					   new il_node::arg_index(mcontext.get_index(this->arguments[pos])),
					   new il_node::arg_pcontext(this->type)));
}

void c_node::c_pcontext::compile_body_delete(const matching_context& mcontext, output_il_context& ocontext) const
{
  // プロセスを引き継ぐのでhead終了時点では何もしない
}

void c_node::c_link::compile_head(matching_context& mcontext, output_il_context& ocontext) const
{
  // これが自由リンクなら何もしないで終わり
  if(this->is_freelink()) return;
  
  // これが既出ならisbuddy出して終わり
  if(mcontext.get_index(this) != -1){
    // 登録だけしてあるアトム引数とisbuddy -> 後で探索し始めたときisbuddy で2回検査してしまうのを回避
    // 先に到達した側でだけ検査する
    if(mcontext.is_isbuddy_checked(this)){
      ocontext.output(new il_node::il_operator("p_isbuddy",
					       new il_node::arg_index(mcontext.get_index(this)),
					       new il_node::arg_index(mcontext.get_index(this->buddy))));
      mcontext.add_isbuddy_checked(this);
      mcontext.add_isbuddy_checked(this->buddy);
    }
    return;
  }
  
  // リンクに番号をふる
  mcontext.add_index(this);
  // OUTPUT: [getbuddy]
  ocontext.output(new il_node::il_operator("getbuddy",
                                           new il_node::arg_index(mcontext.get_index(this)),
                                           new il_node::arg_index(mcontext.get_index(this->buddy))));

  // アトムをcompile_head_child
  this->atomic->compile_head_child(mcontext, ocontext, this->pos);
}




void connect_localfreelinks(std::vector<c_node::c_link*>& freelinks)
{
  // リンク名とその出現回数の対応
  typedef std::map<std::string,std::vector<int> > links_check_type;
  links_check_type links_check;
  // 自由リンクを名前別に振り分ける
  for(unsigned int i=0; i<freelinks.size(); ++i){
    links_check[freelinks[i]->name].push_back(i);
  }
  for(links_check_type::iterator it=links_check.begin(); it!=links_check.end(); ++it){
    std::vector<int>& occurrences = it->second;
    
    // 出現回数をチェック
    if(occurrences.size() > 2){
      std::cerr << "link \""
                << it->first
                << "\" occurrence in scope is "
                << occurrences.size()
                << "."
                << std::endl;
      exit(1);
    }else if(occurrences.size() == 1){
      continue;
    }
    
    // 接続
    c_node::c_link* x = freelinks[occurrences[0]]->buddy;
    c_node::c_link* y = freelinks[occurrences[1]]->buddy;
    // X=Xの形でも問題なく削除できる
    // freelinksの削除した場所にはNULLをつめておき, 後で消す
    x->connect(y);
    delete freelinks[occurrences[0]];
    freelinks[occurrences[0]] = NULL;
    delete freelinks[occurrences[1]];
    freelinks[occurrences[1]] = NULL;
  }
  
  // NULLをまとめて消す
  freelinks.erase(std::remove(freelinks.begin(), freelinks.end(), static_cast<c_node::c_link*>(NULL)), freelinks.end());
}

// グラフ表示中の情報
class output_graph_context
{
public:
  std::ostream& out;
  std::set<c_node::c_atomic*> history;
  linkname_map& named_freelinks;
  int link_count; // 次に使うリンク名
  
  output_graph_context(std::ostream& o, linkname_map& l, int c=0) : out(o), named_freelinks(l), link_count(c) {};
  void set_visited(c_node::c_atomic* x); //xが出力済みであることを記録
  bool is_visited(c_node::c_atomic* x) const; //xが出力済みかどうか
  bool is_named(c_node::c_link* x) const; //xが命名済みかどうか
  std::string make_freelink_occurrence(c_node::c_link* x); // xが出現したことを記録し,その文字列表現を取得
};

void output_graph_context::set_visited(c_node::c_atomic* x)
{
  history.insert(x);
}

bool output_graph_context::is_visited(c_node::c_atomic* x) const
{
  return history.find(x) != history.end();
}

bool output_graph_context::is_named(c_node::c_link* x) const
{
  return named_freelinks.find(x->buddy) != named_freelinks.end();
}

std::string output_graph_context::make_freelink_occurrence(c_node::c_link* x)
{
  std::map<c_node::c_link*,std::string>::iterator it = named_freelinks.find(x);
  
  // すでに1度出現していればその名前を返し,freelinkではなくなったのでリストから消す
  if(it != named_freelinks.end()){
    std::string name = it->second;
    named_freelinks.erase(it);
    return name;
  }

  // 1度目なら名前を作って反対側リンクの名前として登録してから返す
  // ヘッド側自由リンクは自由リンクの反対側が登録されることに注意
  std::stringstream ss;
  ss << "L" << link_count++;
  std::string name = ss.str();
  
  named_freelinks[x->buddy] = name;
  return name;
}

// xの演算子としての強さを返す (値が大きいほど弱い, 演算子でない場合は0を返す)
// 自分より弱い演算子を子として表示する場合は()が必要な場合がある
static int get_op_associative(c_node::c_atomic* x)
{
  if(!x) return 0; // 演算子でなく,リンクとして出力される対象なので0

  if(c_node::c_atom* ax = dynamic_cast<c_node::c_atom*>(x)){
    if(ax->arguments.size() == 3){ // 2項演算子
      if(ax->name=="+" || ax->name=="-"){
        return 2; // + と - 
      }else if(ax->name=="*" || ax->name=="/"){
        return 1; // * と /
      }
    }else if(ax->arguments.size() == 2){ // 単項演算子
      if(ax->name=="+" || ax->name=="-"){
        return 3; //単項演算子は子が演算子でない場合のみ()不要
      }
    }
  }
  return 0; //演算子でなければ0
}

// 親の演算子の強さがparent_op_associativeのとき,子としてxのbuddy側を出力したい場合,括弧が必要かどうか
// 同じ演算子の強さだった場合の返り値はwhen_same_assocになる
static bool is_paren_needed(c_node::c_link* x, int parent_op_associative, output_graph_context& c, bool when_same_assoc)
{
  bool paren_needed = false;
  if(!c.is_named(x)){
    int child_op_associative = get_op_associative(x->buddy->atomic);
    if(child_op_associative > parent_op_associative){
      //リンクに名前がついておらず,リンクの先の方が弱い演算子なら(弱いほうが大きいので注意)括弧が必要
      paren_needed = true;
    }else if(child_op_associative == parent_op_associative){
      //同じ強さだった場合はwhen_same_assocを返す
      paren_needed = when_same_assoc;
    }
  }
  return paren_needed;
}

// アトムを表示, ルートでの出力の場合は最後の引数も出力する
static void output_graph_atomic(c_node::c_atomic* x,
                                output_graph_context& c,
                                bool is_root);

// リンクのatomic側を出力(buddy側ではない)
static void output_graph_link(c_node::c_link* x_l, output_graph_context& c)
{
  if(! c.is_named(x_l)){
    if(x_l->atomic){
      c_node::c_atomic* x = x_l->atomic;
      if(! c.is_visited(x)){
        if(x_l->pos == static_cast<int>(x->arguments.size())-1){
          // 初出の経路で,初出のアトムの最後の引数につながっていた場合は埋め込んで出力
          output_graph_atomic(x, c, false);
          return;
        }
      }
    }
  }
  // その他の場合は自由リンクとして出力
  c.out << c.make_freelink_occurrence(x_l);
  return;
}

static void output_graph_atomic(c_node::c_atomic* x,  // ツリーの根であるアトム
                                output_graph_context& c, // 表示途中の情報
                                bool is_root) // ルートとしての表示か否か(最後の引数を表示するか否か)
{
  c.set_visited(x);

  // 最後の引数を=を使って表示するかどうか 常にfalseでも間違いではない
  // 最後の引数の相手側がグラフ外とつながる自由リンクなら true
  // 最後の引数の相手側がグラフ内とつながる自由リンクなら false
  // 最後の引数の相手側も最後の引数(埋め込める状態)なら true
  bool use_equal = false;
  if(is_root){  // is_rootがtrueでないと=表記は使えない
    if(x->arguments.size() > 0){  // 引数がないと=表記は使えない
      c_node::c_link* last_arg = x->arguments[x->arguments.size()-1];
      if(! last_arg->buddy->atomic){  // 最後の引数が一時的でない完全な自由リンクならatomicはNULLになる
        use_equal = true; //完全な自由リンクなら=を使う
      }else{
        c_node::c_atomic* y = last_arg->buddy->atomic; // yは最後の引数につながっている相手方のアトム
        //yが初出で最後の引数に名前がついていないなら (もしローカルな自由リンク名が来るならa(,,X)の形で埋め込むので=は使わないことにする)
        if(!c.is_visited(y) && !c.is_named(last_arg)){
          if(last_arg->buddy->pos == static_cast<int>(y->arguments.size())-1){ //最後の引数が相手の最後の引数につながっていたら
            use_equal = true;
          }
        }
      }
    }
  }
  
  bool is_special_print_process_done = false; // 何らかの特殊な出力を行い,通常の出力の必要がなければtrue (trueでもequalによる最後の引数の出力は行う)
  
  // リストに対する特殊な処理
  if(!is_root || use_equal){
    if(c_node::c_atom* ax = dynamic_cast<c_node::c_atom*>(x)){
      if(ax->name == "." && ax->arguments.size() == 3){
        c.out << "[";
        output_graph_link(ax->arguments[0]->buddy, c);

        c_node::c_link* tail_l = ax->arguments[1]->buddy;
        while(true){
          if(tail_l->atomic && tail_l->pos == 2){
            if(! c.is_visited(tail_l->atomic)){
              if(c_node::c_atom* tail = dynamic_cast<c_node::c_atom*>(tail_l->atomic)){
                if(tail->name == "." && tail->arguments.size() == 3){
                  // tailの先にまたリスト状にアトムがつながっていたら,リストとして要素を出力
                  c.set_visited(tail);
                  c.out << ","; // リスト要素間の,を出力
                  output_graph_link(tail->arguments[0]->buddy, c);
                  tail_l = tail->arguments[1]->buddy;
                  continue;
                }
              }
            }
          }
          break;
        }

        {
          bool list_closed_flag = false; // 入れ子になったifをうまく抜ける方法が無かったので
          if(tail_l->atomic && tail_l->pos == 0){
            if(c_node::c_atom* tail = dynamic_cast<c_node::c_atom*>(tail_l->atomic)){
              if(tail->name == "[]" && tail->arguments.size() == 1){
                // 末尾がちゃんと[]で終端してあったら
                c.set_visited(tail);
                c.out << "]"; // リストを終端
                list_closed_flag = true;
              }
            }
          }
          if(! list_closed_flag){
            c.out << "|"; // リストの残り部分を出力
            output_graph_link(tail_l, c);
            c.out << "]"; // リストを終端
          }
        }

        is_special_print_process_done = true;
      }
    }
  }
  // ルート出現の+x,-Yに対する特殊な処理
  if(! is_special_print_process_done && is_root){
    if(c_node::c_atom* ax = dynamic_cast<c_node::c_atom*>(x)){
      if((ax->name=="+"||ax->name=="-") && ax->arguments.size() == 1){
        c.out << ax->name;
        output_graph_link(ax->arguments[0]->buddy, c);
        return;
      }
    }
  }
  // 数式的なアトムに対する特殊な処理
  if(! is_special_print_process_done && !is_root){
    if(c_node::c_atom* ax = dynamic_cast<c_node::c_atom*>(x)){
      int op_associative = get_op_associative(x); // 演算子の結合性 0なら演算子ではない
      if(op_associative > 0){
        if(ax->arguments.size() == 2){ //単項演算子
          //子の出力に括弧が必要かどうか 同じ強さなら括弧は不要
          bool paren_needed = is_paren_needed(ax->arguments[0], op_associative, c, false);
          c.out << ax->name;
          if(paren_needed) c.out << "(";
          output_graph_link(ax->arguments[0]->buddy, c);
          if(paren_needed) c.out << ")";
          is_special_print_process_done = true;
        }else if(ax->arguments.size() == 3){ //2項演算子
          //左側の子の出力に括弧が必要かどうか 同じ強さなら括弧は不要
          bool lhs_paren_needed = is_paren_needed(ax->arguments[0], op_associative, c, false);
          if(lhs_paren_needed) c.out << "(";
          output_graph_link(ax->arguments[0]->buddy, c);
          if(lhs_paren_needed) c.out << ")";
          c.out << ax->name;
          //右側の子の出力に括弧が必要かどうか 同じ強さなら括弧が必要
          bool rhs_paren_needed = is_paren_needed(ax->arguments[1], op_associative, c, true);
          if(rhs_paren_needed) c.out << "(";
          output_graph_link(ax->arguments[1]->buddy, c);
          if(rhs_paren_needed) c.out << ")";
          is_special_print_process_done = true;
        }
      }
    }
  }
  
  if(! is_special_print_process_done){
    // 名前の出力
    c.out << x->name_string();
    // 括弧の中で出力する引数の個数
    int arg_in_paren = x->arguments.size() - ((!is_root || use_equal)?1:0);
    // 括弧内の引数があれば括弧を出力
    if(arg_in_paren > 0) c.out << "(";
    // 括弧内の引数を出力
    for(int i=0; i<arg_in_paren; ++i){
      if(i != 0) c.out << ",";  //引数の間の,を出力
      output_graph_link(x->arguments[i]->buddy, c);
    }
    if(arg_in_paren > 0) c.out << ")";
  }
  
  // =を使うことにしていた場合は最後の引数を出力
  if(use_equal){
    c.out << "=";
    output_graph_link(x->arguments[x->arguments.size()-1]->buddy, c);
  }
}

// xの根としての優先度を返す(高いほど優先する)
static int c_atomic_priority_as_root(const c_node::c_atomic* x)
{
  if(const c_node::c_atom* ax = dynamic_cast<const c_node::c_atom*>(x)){
    if(ax->arguments.size() == 0){
      return 10; //無引数の場合
    }
    if(ax->name == "." && ax->arguments.size() == 3){
      return 3; //consアトムの場合
    }
    if(ax->name == "[]" && ax->arguments.size() == 1){
      return 3; //nilアトムの場合
    }
  }else{
    return 0; //アトム以外(整数など)の場合
  }
  
  // 自分の最後のリンクに相手の最後以外のリンクがつながっている場合は優先度が低い (相手側に埋め込みたい)
  {
    c_node::c_link* last_buddy = x->arguments[x->arguments.size()-1]->buddy;
    if(last_buddy->atomic){
      c_node::c_atomic* y = last_buddy->atomic;
      if(last_buddy->pos != static_cast<int>(y->arguments.size())-1){
        return 2;
      }
    }
  }
  
  return 5; //上のどれでもない場合
}

// 根として優れている順に並んでいるか
static bool c_atomic_greater_as_root(const c_node::c_atomic* lhs, const c_node::c_atomic* rhs)
{
  return c_atomic_priority_as_root(lhs) > c_atomic_priority_as_root(rhs);
}

void output_graph(std::ostream& o,
                  const std::vector<c_node::c_atomic*>& a,
                  std::map<c_node::c_link*,std::string>& l,
                  int& first_link_id)
{
  std::vector<c_node::c_atomic*> atomics = a;
  std::sort(atomics.begin(), atomics.end(), c_atomic_greater_as_root);
  
  output_graph_context context(o, l, first_link_id);
  bool output_firsttime = true; //一番最初の出力時だけ
  
  for(unsigned int i=0; i<atomics.size(); ++i){ // TODO:アトムより先に自由リンクを根に出力すべき
    c_node::c_atomic* x = atomics[i];
    
    if(context.is_visited(x)) continue; // すでに出力済みのアトム
    if(! output_firsttime) context.out << ", "; // ツリーとツリーの間の,を出力
    
    output_graph_atomic(x, context, true);
    output_firsttime = false;
  }
  
  // 自由リンク同士がつながっていたら=文を出力 (ルールのbodyで必要)
  // O(自由リンク数^2) なので直した方がいいかもしれない
  for(linkname_map::iterator it=l.begin(); it!=l.end(); ){
    linkname_map::iterator found_it_buddy = l.find(it->first->buddy); // 自由リンクの反対側を自由リンクの中から探す
    if(found_it_buddy != l.end()){
      if(! output_firsttime) context.out << ", "; // グラフ間の,を出力
      context.out << it->second << "=" << found_it_buddy->second; // 接続文を出力
      // 接続して自由リンクではなくなったので自由リンクの表からは消す
      l.erase(it++); // 消した要素へのイテレータは無効になるので,消す前にインクリメントする必要がある
      l.erase(found_it_buddy); // 連想コンテナは消した要素を指すイテレータのみを無効とするのでそのまま消してOK
      output_firsttime = false;
    }else{
      ++it;
    }
  }
  
  //次のリンクidの情報を返す
  first_link_id = context.link_count;
}

