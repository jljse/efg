#include <iostream>
#include <map>
#include <queue>
#include <stack>
#include <cassert>
#include <algorithm>
#include "compiler.h"
#include "p_to_c.h"
#include "c_to_il.h"
#include "c_node.h"
#include "p_node.h"
#include "il_node.h"

using namespace std;

c_node::c_root* compiler::to_c_node(const p_node::p_root* src_root)
{
  p_to_c_context grobal_scope;
  src_root->to_c_node_as_root(grobal_scope);
  
  c_node::c_root* dst_root = new c_node::c_root;
  dst_root->rules = grobal_scope.rules;
  dst_root->atomics = grobal_scope.atomics;
  
  return dst_root;
}

// ヘッド側のマッチング命令を生成
static void compile_head(matching_context& mcontext,
                         output_il_context& ocontext,
                         const c_node::c_rule* rule)
{
  for(unsigned int root_index=0; root_index<rule->head.size(); ++root_index){
    c_node::c_atomic* root = rule->head[root_index];
    // マッチ済みなら飛ばす
    if(mcontext.get_index(root) != -1) continue;
    // ルートになれないなら飛ばす
    if(! root->is_compile_head_root()) continue;
    // rootからたどれる範囲をマッチする
    root->compile_head_root(mcontext, ocontext);
  }
  
  // データも全部マッチできたか確認 何か残っていたらマッチのしようがないルールとしてエラーにする
  {
    bool error_flag = false;
    for(unsigned int i=0; i<rule->head.size(); ++i){
      c_node::c_atomic* x = rule->head[i];
      if(mcontext.get_index(x) == -1){
        // マッチできていないアトムがある シンボルだけならfindatomできるのでありえない
        cerr << "there is atom never matched " << x->name_string() << " at " << x->loc << "." << endl;
        error_flag = true;
      }
    }
    if(error_flag) exit(1);
  }
}

// ヘッド側でマッチしたアトムとリンクを削除する
static void compile_body_delete(const matching_context& mcontext,
                                output_il_context& ocontext)
{
  // マッチした局所リンクを処理する TODO: 遅い
  for(matching_context::found_link_type::const_iterator it=mcontext.found_link.begin();
      it!=mcontext.found_link.end();
      ++it){
    // 局所リンクを削除 自由リンクにつながっているリンクは使いまわす
    if(! it->first->buddy->is_freelink()){
      // OUTPUT: [freelink]
      ocontext.output(new il_node::il_operator("freelink", new il_node::arg_index(it->second)));
    }
  }
  
  // マッチしたアトムのうち消せるものを削除する
  for(matching_context::found_atom_type::const_iterator it=mcontext.found_atom.begin();
      it!=mcontext.found_atom.end();
      ++it){
    it->first->compile_body_delete(mcontext, ocontext);
  }
}

// ボディ側で必要になるリンクを生成/接続する
static void compile_body_ready_links(matching_context& mcontext,
                                     output_il_context& ocontext,
                                     const vector<pair<c_node::c_link*,c_node::c_link*> >& freelinks,
                                     const vector<c_node::c_atomic*>& body)
{
  // 自由リンクを処理する
  for(unsigned int i=0; i<freelinks.size(); ++i){
    // 自由リンクである場合アトム部を消し,body側のリンクとして再登録
    mcontext.add_index(freelinks[i].second->buddy);
    // OUTPUT: [unconnect]
    //cout << "unconnect [" << mcontext.get_index(freelinks[i].second->buddy) << ","
    //     << mcontext.get_index(freelinks[i].first->buddy) << "]" << endl;
    ocontext.output(new il_node::il_operator("unconnect",
                                             new il_node::arg_index(mcontext.get_index(freelinks[i].second->buddy)),
                                             new il_node::arg_index(mcontext.get_index(freelinks[i].first->buddy))));
  }

  // buddyを登録しておくための配列
  vector<pair<int,int> > buddies;
  
  // ボディ側で使うリンクを生成する
  for(unsigned int i=0; i<body.size(); ++i){
    c_node::c_atomic* x = body[i];
    for(unsigned int j=0; j<x->arguments.size(); ++j){
      if(mcontext.get_index(x->arguments[j]) == -1){
        // その引数のリンクが未生成なら
        mcontext.add_index(x->arguments[j]);
        // OUTPUT: [newlink]
        //cout << "newlink [" << mcontext.get_index(x->arguments[j]) << "]" << endl;
        ocontext.output(new il_node::il_operator("newlink", new il_node::arg_index(mcontext.get_index(x->arguments[j]))));
      }
      if(mcontext.get_index(x->arguments[j]->buddy) != -1){
        // 相方も生成済みならbuddyとして登録
        buddies.push_back(make_pair(mcontext.get_index(x->arguments[j]),
                                    mcontext.get_index(x->arguments[j]->buddy)));
      }
    }
  }
  
  // リンクのbuddyをつなぐ
  for(unsigned int i=0; i<buddies.size(); ++i){
    // OUTPUT: [bebuddy]
    //cout << "bebuddy [" << buddies[i].first << ","
    //     << buddies[i].second << "]" << endl;
    ocontext.output(new il_node::il_operator("bebuddy", new il_node::arg_index(buddies[i].first), new il_node::arg_index(buddies[i].second)));
  }

  // 自由リンク同士の接続
  for(unsigned int i=0; i<freelinks.size(); ++i){
    c_node::c_link* x = freelinks[i].second;
    for(unsigned int j=i; j<freelinks.size(); ++j){
      c_node::c_link* y = freelinks[j].second;
      if(x->buddy == y){
        // 自由リンク同士の接続だった場合 (TODO:getbuddy,getbuddy,bebuddy,freelink,freelinkと同じじゃないか？)
        // OUTPUT: [unify]
        //cout << "unify [" << mcontext.get_index(x) << ","
        //     << mcontext.get_index(y) << "]" << endl;
        ocontext.output(new il_node::il_operator("unify",
                                                 new il_node::arg_index(mcontext.get_index(x)),
                                                 new il_node::arg_index(mcontext.get_index(y))));
      }
    }
  }
}

// ボディ側のデータやプロセス文脈の生成を行う(callされるもの以外全部)
// TODO: プロセス文脈の引き継ぎとそれ以外(整数含む)にわけたほうがよくないか
static void compile_body_ready_data(matching_context& mcontext,
                                    output_il_context& ocontext,
                                    const vector<pair<c_node::c_pcontext*,vector<c_node::c_pcontext*> > >& pcontexts,
                                    const vector<c_node::c_atomic*>& body)
{
  for(int i=static_cast<int>(body.size())-1; i>=0; --i){
    if(c_node::c_pcontext* pcontext = dynamic_cast<c_node::c_pcontext*>(body[i])){
      // TODO: プロセス文脈の場合

      // body側配列の0個目はmove, その他はcopy (moveしてからcopyもありうる)
      for(unsigned int j=0; j<pcontexts.size(); ++j){
        if(pcontexts[j].first->name == pcontext->name){
          // 対応情報を発見
          for(unsigned int k=0; k<pcontexts[j].second.size(); ++k){
            if(pcontexts[j].second[k] == pcontext){
              // 自分を発見
              if(k == 0){
                // move
                ocontext.output(new il_node::il_operator("movepc",
                                                         new il_node::arg_index(mcontext.get_index(pcontext->arguments[0])),
                                                         new il_node::arg_index(mcontext.get_index(pcontexts[j].first)),
                                                         new il_node::arg_pcontext(pcontexts[j].first->type)));
              }else{
                // copy
                ocontext.output(new il_node::il_operator("copypc",
                                                         new il_node::arg_index(mcontext.get_index(pcontext->arguments[0])),
                                                         new il_node::arg_index(mcontext.get_index(pcontexts[j].first)),
                                                         new il_node::arg_pcontext(pcontexts[j].first->type)));
              }
            }
          }
        }
      }
    }else if(c_node::c_integer* integer = dynamic_cast<c_node::c_integer*>(body[i])){
      int tempvarindex = mcontext.add_index();
      // 整数アトムの場合
      ocontext.output(new il_node::il_operator("newint",
                                               new il_node::arg_index(tempvarindex),
                                               new il_node::arg_integer(integer->data)));
      ocontext.output(new il_node::il_operator("setint",
                                               new il_node::arg_index(mcontext.get_index(integer->arguments[0])),
                                               new il_node::arg_index(tempvarindex)));
    }
  }
  
  // 引き継ぎのなかったプロセス文脈を削除
  for(unsigned int i=0; i<pcontexts.size(); ++i){
    if(pcontexts[i].second.size() == 0){
      ocontext.output(new il_node::il_operator("freepc",
                                               new il_node::arg_index(mcontext.get_index(pcontexts[i].first)),
                                               new il_node::arg_pcontext(pcontexts[i].first->type)));
    }
  }
}

// ボディ側の各アトムに対して呼び出しを行う 順番は入出力関係が分かるまでとりえあえず逆順
static void compile_body_call_atoms(const matching_context& mcontext,
                                    output_il_context& ocontext,
                                    const vector<c_node::c_atomic*> body)
{
  for(int i=static_cast<int>(body.size())-1; i>=0; --i){
    if(c_node::c_atom* func = dynamic_cast<c_node::c_atom*>(body[i])){
      // 非データアトムの場合
      // OUTPUT: [call]
      il_node::arg_array* arg = new il_node::arg_array;
      //cout << "call [" << func->name << ":" << func->arguments.size() << ",";
      //cout << "[[";
      for(unsigned int j=0; j<func->arguments.size(); ++j){
        //if(j != 0) cout << ",";
        //cout << mcontext.get_index(func->arguments[j]);
        arg->array.push_back(new il_node::arg_index(mcontext.get_index(func->arguments[j])));
      }
      //cout << "]]";
      //cout << "]" << endl;
      ocontext.output(new il_node::il_operator("call",
                                               new il_node::arg_functor(func->name, func->arguments.size()),
                                               arg));
    }
  }
}

// ルールをコンパイルする
static void compile_rule(const c_node::c_rule* rule, output_il_context& ocontext)
{
  // マッチングの状態
  matching_context mcontext;

  ocontext.begin_rule();

  // ヘッド側をコンパイル
  compile_head(mcontext, ocontext, rule);

  // OUTPUT: [commit]
  // cout << "commit []" << endl;
  ocontext.output(new il_node::il_operator("commit"));
  
  // マッチした構造を消す
  compile_body_delete(mcontext, ocontext);
  // ボディ側で必要になるリンクを生成/接続する
  compile_body_ready_links(mcontext, ocontext, rule->freelinks, rule->body);
  // データアトム/プロセス文脈等を生成する
  compile_body_ready_data(mcontext, ocontext, rule->pcontexts, rule->body);
  // 各アトムをcallする 
  compile_body_call_atoms(mcontext, ocontext, rule->body);
  // TODO: データ=データの形をどうするのか？ それができてしまうと絶対リークする&表示もできない

  // OUTPUT: [success]
  // cout << "success []" << endl;
  ocontext.output(new il_node::il_operator("success"));
  
  //cout << endl;

  ocontext.end_rule();
}

// 初期データをコンパイルする
static void compile_initial(const std::vector<c_node::c_atomic*> atomics, std::list<il_node::il_operator*>& out)
{
  // マッチングの状態
  matching_context mcontext;
  // 出力の状態
  output_il_context ocontext;
  
  ocontext.begin_rule();
  
  // ボディ側で必要になるリンクを生成/接続する
  // 自由リンクはないのでfreelinksを一時的に作る(これにはなにも追加されないはず)
  std::vector<std::pair<c_node::c_link*,c_node::c_link*> > freelinks;
  compile_body_ready_links(mcontext, ocontext, freelinks, atomics);
  
  // データアトムを生成する
  // プロセス文脈はないのでpcontextsを一時的に作る
  std::vector<std::pair<c_node::c_pcontext*,std::vector<c_node::c_pcontext*> > > pcontexts;
  compile_body_ready_data(mcontext, ocontext, pcontexts, atomics);
  // 各アトムをcallする 
  compile_body_call_atoms(mcontext, ocontext, atomics);

  ocontext.end_rule();
  
  // ここでocontextには初期データ生成用のルールが1つだけ入っているので取り出す
  std::vector<std::list<il_node::il_operator*>*> temp;
  ocontext.swap_result(temp);
  
  // 取り出したルールをさらにoutに出力する
  out.clear(); // 一応クリア
  out.splice(out.end(), *temp[0]);
}

static void compile_binop_rule_int(output_il_context& ocontext, std::string src_op, std::string il_op)
{
  // マッチングの状態
  matching_context mcontext;

  ocontext.begin_rule();
  // OUTPUT: [findatom]
  int vplus = mcontext.add_index();
  il_node::arg_block* block = new il_node::arg_block;
  ocontext.output_begin_block(new il_node::il_operator("p_findatom",
                                                       new il_node::arg_index(vplus),
                                                       new il_node::arg_functor(src_op, 3),
                                                       block),
                              &(block->block));
  // OUTPUT: [getlink]
  int v0 = mcontext.add_index();
  ocontext.output(new il_node::il_operator("getlink",
                                           new il_node::arg_index(v0),
                                           new il_node::arg_index(vplus),
                                           new il_node::arg_integer(0)));
  // OUTPUT: [getlink]
  int v1 = mcontext.add_index();
  ocontext.output(new il_node::il_operator("getlink",
                                           new il_node::arg_index(v1),
                                           new il_node::arg_index(vplus),
                                           new il_node::arg_integer(1)));
  // OUTPUT: [getlink]
  int v2 = mcontext.add_index();
  ocontext.output(new il_node::il_operator("getlink",
                                           new il_node::arg_index(v2),
                                           new il_node::arg_index(vplus),
                                           new il_node::arg_integer(2)));
  
  int v0b = mcontext.add_index();
  ocontext.output(new il_node::il_operator("getbuddy",
                                           new il_node::arg_index(v0b),
                                           new il_node::arg_index(v0)));
  // OUTPUT: [getlink]
  int v1b = mcontext.add_index();
  ocontext.output(new il_node::il_operator("getbuddy",
                                           new il_node::arg_index(v1b),
                                           new il_node::arg_index(v1)));

  // OUTPUT: [p_derefint]
  int v0i = mcontext.add_index();
  ocontext.output(new il_node::il_operator("p_derefint",
                                           new il_node::arg_index(v0i),
                                           new il_node::arg_index(v0b)));

  // OUTPUT: [p_derefint]
  int v1i = mcontext.add_index();
  ocontext.output(new il_node::il_operator("p_derefint",
                                           new il_node::arg_index(v1i),
                                           new il_node::arg_index(v1b)));

  // OUTPUT: [commit]
  // cout << "commit []" << endl;
  ocontext.output(new il_node::il_operator("commit"));

  // OUTPUT: [intadd]
  int v2i = mcontext.add_index();
  ocontext.output(new il_node::il_operator(il_op,
                                           new il_node::arg_index(v2i),
                                           new il_node::arg_index(v0i),
                                           new il_node::arg_index(v1i)));

  // OUTPUT: [setint]
  ocontext.output(new il_node::il_operator("setint",
                                           new il_node::arg_index(v2),
                                           new il_node::arg_index(v2i)));

  // OUTPUT: [success]
  ocontext.output(new il_node::il_operator("success"));

  ocontext.end_rule();
}

static void compile_basic_rule_intadd(output_il_context& ocontext)
{
  compile_binop_rule_int(ocontext, "+", "intadd");
  compile_binop_rule_int(ocontext, "-", "intsub");
  compile_binop_rule_int(ocontext, "*", "intmul");
  compile_binop_rule_int(ocontext, "/", "intdiv");
  compile_binop_rule_int(ocontext, "mod", "intmod");
  compile_binop_rule_int(ocontext, "==", "inteq");
  compile_binop_rule_int(ocontext, ">", "intgt");
}

static void compile_basic_rules(output_il_context& ocontext)
{
  compile_basic_rule_intadd(ocontext);
}

il_node::il_root* compiler::compile(const c_node::c_root* src_root)
{
  // 最終的な出力
  il_node::il_root* result = new il_node::il_root;

  // 変換した出力を受ける
  output_il_context ocontext;
  for(unsigned int i=0; i<src_root->rules.size(); ++i){
    // src_root->rules[i]->output(cout);
    // cout << endl;
    compile_rule(src_root->rules[i], ocontext);
  }
  // 組み込みルール
  compile_basic_rules(ocontext);

  ocontext.swap_result(result->rules);


  // 初期データ生成ルールを受け取る
  compile_initial(src_root->atomics, result->initial);
  
  return result;
}

