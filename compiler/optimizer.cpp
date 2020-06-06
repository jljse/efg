#include "optimizer.h"
#include <set>
#include <vector>
#include <deque>
#include <string>
#include <cassert>
#include <algorithm>
#include <iostream>
#include "il_node.h"
#include "utility.h"

// 一部で使う略称(使っていないところも多いので注意
typedef std::list<il_node::il_operator*> il_operator_list;

static void optimize_reusing(il_operator_list& block,
                             std::vector<std::pair<il_operator_list*,il_operator_list::iterator> >& freelink_operations);

// 再帰するためだけの関数
static void optimize_reusing(il_node::arg_base* arg,
                             std::vector<std::pair<il_operator_list*,il_operator_list::iterator> >& freelink_operations)
{
  if(il_node::arg_block* block = dynamic_cast<il_node::arg_block*>(arg)){
    optimize_reusing(block->block, freelink_operations);
  }else if(il_node::arg_array* array = dynamic_cast<il_node::arg_array*>(arg)){
    for(unsigned int i=0; i<array->array.size(); ++i){
      optimize_reusing(array->array[i], freelink_operations);
    }
  }
}

// あるブロック内に関して再利用
// 外でdeleteしてブロックの中でnewする可能性があるので一応情報を持ちまわす
// eraseの際にリスト自体も必要なので持ちまわす
// TODO: branchのような命令があると複数のブロックで別々の変数状況になるので修整が必要
//       今はmergingより先にやることを期待している
static void optimize_reusing(il_operator_list& block,
                             std::vector<std::pair<il_operator_list*,il_operator_list::iterator> >& freelink_operations)
{
  for(il_operator_list::iterator it = block.begin();
      it != block.end();
      ){
    /* 削除する場合があるのでここではfor文としてはイテレータは進めない */
    bool iterate_flag = true;
    il_node::il_operator* op = *it;
    
    if(op->opecode == "freelink"){
      // リンクの削除命令があったら追加
      freelink_operations.push_back(std::pair<il_operator_list*,il_operator_list::iterator>(&block, it));
    }else if(op->opecode == "newlink"){
      // リンク生成命令があったら
      if(freelink_operations.size() > 0){
        // それまでに削除されたリンクがあるなら再利用する
        
        // 削除するfreelink命令
        il_node::il_operator* freelink_op = *freelink_operations.back().second;
        // freelinkで削除するリンクのインデックス
        int freelink_index = dynamic_cast<il_node::arg_index*>(freelink_op->arguments[0])->index;
        // newlinkで生成するリンクのインデックス
        int newlink_index = dynamic_cast<il_node::arg_index*>(op->arguments[0])->index;
        
        // OUTPUT: [unconnect]をnewlinkのあった位置に挿入
        block.insert(it, new il_node::il_operator("unconnect", new il_node::arg_index(newlink_index),
                                                               new il_node::arg_index(freelink_index)));

        // freelink命令をルールから外す削除
        freelink_operations.back().first->erase(freelink_operations.back().second);
        // newlink命令をルールから外す 戻り値が次のイテレータ
        it = block.erase(it);
        // freelink命令を消す
        delete freelink_op;
        // newlink命令を消す
        delete op;
        
        // 使ったfreelink命令を使いまわしリストから外す
        freelink_operations.pop_back();
        // eraseしたのでイテレータは進めない
        iterate_flag = false;
      }
    }else{
      // それ以外の命令なら命令の引数についてチェック
      for(unsigned int i=0; i<op->arguments.size(); ++i){
        optimize_reusing(op->arguments[i], freelink_operations);
      }
    }
    // itが削除される場合ここでは進めない
    if(iterate_flag){
      ++it;
    }
  }
}

// 再利用に関する最適化
static void optimize_reusing(std::vector<std::list<il_node::il_operator*>*>& rules)
{
  // TODO:アトムの再利用はもっとちゃんと考える必要がある気がするのでパス
  //      (ルール関数を呼ぶ前後にまたがってアトムを再利用できれば嬉しい->相当実装変わるけど)
  // [freelink X] -> [newlink Y] を見つけて [unconnect Y X] にする
  // mergingより前に実行されることを期待している (branchなし,完全入れ子,notとかも考えていない)
  
  for(unsigned int i=0; i<rules.size(); ++i){
    // freelinkの場所を覚えておく
    std::vector<std::pair<il_operator_list*,il_operator_list::iterator> > freelink_operations;
    optimize_reusing(*rules[i], freelink_operations);
  }
}

static void optimize_merging(std::vector<std::list<il_node::il_operator*>*>& rules);

class il_block_group
{
public:
  // マージ可能なグループ
  std::vector<std::list<il_node::il_operator*>*> group;

  // このグループに追加してよい命令列かどうか
  bool is_group(const std::list<il_node::il_operator*>* x);
  // このグループに追加する xは破壊されるかもしれない
  void add(std::list<il_node::il_operator*>* x);
  // このグループをマージする
  void merge();
  // マージ後の結果を返す
  std::list<il_node::il_operator*>* get_result();
};

bool il_block_group::is_group(const std::list<il_node::il_operator*>* x)
{
  return group[0]->front()->is_same_shallow(x->front());
}

void il_block_group::add(std::list<il_node::il_operator*>* x)
{
  group.push_back(new std::list<il_node::il_operator*>());
  group.back()->splice(group.back()->end(), *x);
}

// blocksの複数のブロックをbranch命令の下に入れる
static void to_branch(std::vector<std::list<il_node::il_operator*>*>& blocks)
{
  // マージ結果を処理する
  if(blocks.size() > 1){
    // 2つ以上のグループになっていればbranchの下にくっつける
    il_node::arg_array* new_array = new il_node::arg_array();
    for(unsigned int i=0; i<blocks.size(); ++i){
      // 各グループのためのblockを作りその下にくっつける
      il_node::arg_block* new_block = new il_node::arg_block();
      new_block->block.splice(new_block->block.end(), *blocks[i]);
      // できたblockをarrayに追加
      new_array->array.push_back(new_block);
    }
    // 全部追加したのでblocksはクリアした後,branch命令用の要素1つだけ用意する
    blocks.clear();
    blocks.push_back(new std::list<il_node::il_operator*>());
    // OUTPUT: [branch]
    blocks[0]->push_front(new il_node::il_operator("p_branch", new_array));
  }
}

// arg_blockのvectorからvector<list<il_operator*>*>にする
static void convert_arg_blocks(std::vector<il_node::arg_block*>& from,
                               std::vector<std::list<il_node::il_operator*>*>& to)
{
  // 一応
  assert(to.size() == 0);
  to.resize(from.size());
  for(unsigned int i=0; i<from.size(); ++i){
    to[i] = new std::list<il_node::il_operator*>();
    to[i]->splice(to[i]->end(), from[i]->block);
  }
  std::for_each(from.begin(), from.end(), deleter());
  from.clear();
}

void il_block_group::merge()
{
  if(group.size() > 1){
    // block引数を含む命令のマージは個別に行う必要がある
    if(group[0]->front()->opecode == "p_findatom"){
      // 各ブロックの入れ子部分を集める
      std::vector<il_node::arg_block*> temp(group.size());
      for(unsigned int i=0; i<group.size(); ++i){
        // 入れ子部分は2番目の引数
        temp[i] = dynamic_cast<il_node::arg_block*>(group[i]->front()->arguments[2]);
        group[i]->front()->arguments[2] = 0;
      }
      // 余分なfindatomを消す
      for(unsigned int i=1; i<group.size(); ++i){
        delete group[i];
      }
      // 結果のブロックは1つにする
      group.resize(1);

      // 各ブロックの入れ子部分
      std::vector<std::list<il_node::il_operator*>*> blocks;
      convert_arg_blocks(temp, blocks);
      
      // それぞれの内部blockについてoptimize_mergingを呼ぶ;
      optimize_merging(blocks);
      // マージした結果をbranchにする
      to_branch(blocks);
      
      // その結果を全体の内部blockとして戻す.
      il_node::arg_block* new_block = new il_node::arg_block();
      new_block->block.splice(new_block->block.end(), *blocks[0]);
      group[0]->front()->arguments[2] = new_block;
    }else{
      // block引数のない命令のマージ

      // 余分な先頭を消す
      for(unsigned int i=1; i<group.size(); ++i){
        il_node::il_operator* op = group[i]->front();
        group[i]->pop_front();
        delete op;
      }
      
      // 全体の新しい先頭命令
      il_node::il_operator* new_root = group[0]->front();
      group[0]->pop_front();
      
      // それぞれの2番目以下の命令についてマージを行う
      optimize_merging(group);
      // マージしきれなかったものはbranchの下に入れる
      to_branch(group);
      
      // 先頭によけておいた命令を追加
      group[0]->push_front(new_root);
    }
  }
}

std::list<il_node::il_operator*>* il_block_group::get_result()
{
  // きちんとマージされているか一応確認
  assert(group.size() == 1);
  return group[0];
}

// ilに対して編み上げ最適化を行う 入力は命令列の集合,出力も命令列の集合
class optimize_il_merger
{
public:
  // splitting[0] は 編み上げたい1グループ
  std::vector<il_block_group*> splitting;

  // 命令列を編み上げ対象として追加する (結果を受け取るまで渡した命令列に触ってはいけない)
  void add(std::list<il_node::il_operator*>* block);
  // これまで追加された命令列を編み上げる
  void merge();
  // 編み上げた結果をresultに受け取る
  void get_result(std::vector<std::list<il_node::il_operator*>*>& result);
};

void optimize_il_merger::add(std::list<il_node::il_operator*>* block)
{
  // addの時点では浅い比較をして集めるだけでよい
  // [not]命令を追加する場合は何か変える必要がある

  for(unsigned int i=0; i<splitting.size(); ++i){
    // 先頭の命令が一致した場合,そこに追加する
    if(splitting[i]->is_group(block)){
      splitting[i]->add(block);
      return;
    }
  }
  
  // どれとも一致しなければ,最後に追加する
  splitting.push_back(new il_block_group);
  splitting.back()->add(block);
}

void optimize_il_merger::merge()
{
  for(unsigned int i=0; i<splitting.size(); ++i){
    // それぞれのグループをマージする
    splitting[i]->merge();
  }
}

void optimize_il_merger::get_result(std::vector<std::list<il_node::il_operator*>*>& result)
{
  // 戻す先を全部空にする
  result.clear();
  result.resize(splitting.size());
  for(unsigned int i=0; i<splitting.size(); ++i){
    // 個々のグループの結果を受け取る
    result[i] = splitting[i]->get_result();
  }
}

// ルール/ブロックの編み上げを行う
static void optimize_merging(std::vector<std::list<il_node::il_operator*>*>& rules)
{
  // 超手抜き実装
  //   ブロックはfindatomのみ
  //   変数の振替は行わない

  // デバッグプリント
//   std::cout << ">>merging" << std::endl;
//   for(unsigned int i=0; i<rules.size(); ++i){
//     for(std::list<il_node::il_operator*>::iterator it = rules[i]->begin();
//         it != rules[i]->end();
//         ++it){
//       (*it)->output(std::cout, 2);
//     }
//     std::cout << std::endl;
//   }
//   std::cout << "<<merging" << std::endl;
  
  
  optimize_il_merger oim;

  for(unsigned int i=0; i<rules.size(); ++i){
    if(rules[i]->size() > 0){
      // 中身のあるブロックなら追加する (中身がなくなる==完全に同じ？)
      oim.add(rules[i]);
    }
  }
  oim.merge();

  // 結果を受け取る
  oim.get_result(rules);
}

static void delete_operator(std::list<il_node::il_operator*>* block, const il_node::il_operator* op);

static void delete_operator(il_node::arg_base* arg, const il_node::il_operator* op)
{
  if(il_node::arg_block* block = dynamic_cast<il_node::arg_block*>(arg)){
    // もし引数がarg_blockなら
    delete_operator(&block->block, op);
  }if(il_node::arg_array* array = dynamic_cast<il_node::arg_array*>(arg)){
    // もし引数がarg_arrayなら
    for(unsigned int i=0; i<array->array.size(); ++i){
      delete_operator(array->array[i], op);
    }
  }
}

// block内に出現する命令opを削除する (消す命令は入れ子付きでないもの)
// opにはワイルドカードを含めることができる
static void delete_operator(std::list<il_node::il_operator*>* block, const il_node::il_operator* op)
{
  for(std::list<il_node::il_operator*>::iterator it = block->begin();
      it != block->end();
      ){
    // opにはワイルドカードが含まれるかもしれないのでop->から呼び出す
    if( op->is_same_shallow(*it) ){
      // もし同じ命令なら (当然入れ子命令ではない) 消す
      delete *it;
      it = block->erase(it); //ここでイテレータが進むので注意
      continue;
    }else{
      for(unsigned int i=0; i<(*it)->arguments.size(); ++i){
        delete_operator((*it)->arguments[i], op);
      }
    }

    ++it;
  }
}

// ruleに対し, 最初のfindatomで拾うはずのアトム(ファンクタはfunc)を再生する処理を追加する funcは使わないで新しくnewする
// 再生用branchとルールの無いアトム用ルールに使われる
static void add_first_atom_create_operators(std::list<il_node::il_operator*>* rule,
                                            il_node::arg_functor* func,
                                            bool is_enqueue)
{
  // OUTPUT: [newatom]
  rule->push_back(new il_node::il_operator("newatom", new il_node::arg_index(0), new il_node::arg_functor(func->name, func->arity)));
  for(int i=0; i<func->arity; ++i){
    // 各引数をつなぐ
    // OUTPUT: [setlink]
    rule->push_back(new il_node::il_operator("setlink",
                                             new il_node::arg_index(i+1),
                                             new il_node::arg_index(0),
                                             new il_node::arg_integer(i)));
  }
  // OUTPUT: [enqueue]
  if(is_enqueue){
    rule->push_back(new il_node::il_operator("enqueue", new il_node::arg_index(0)));
  }
  // OUTPUT: [failure]
  rule->push_back(new il_node::il_operator("failure"));

}

// ruleに対し, その外にfuncの失敗時再生用branchをつける
static void add_resume_branch(std::list<il_node::il_operator*>* rule, il_node::arg_functor* func)
{
  if(rule->front()->opecode == "commit"){
    // 最初の命令がcommitだった場合, 必ず成功するので失敗時処理不要
    return;
  }
  
  // 再生用のルールを入れる この再生したアトムは再度始点として試す必要がある
  il_node::arg_block* resumeblock = new il_node::arg_block;
  add_first_atom_create_operators(&resumeblock->block, func, true);
  
  if(rule->front()->opecode == "p_branch"){
    // 最初の命令がbranchだった場合, その枝の1つに再生用ルールを入れる
    il_node::il_operator* branch = rule->front();
    il_node::arg_array* array = static_cast<il_node::arg_array*>(branch->arguments[0]);
    array->array.push_back(resumeblock);
  }else{
    // そうでなければ全体の外側にbranchをくっつける
    // メインのルールを入れる
    il_node::arg_block* mainblock = new il_node::arg_block;
    mainblock->block.splice(mainblock->block.end(), *rule);
    // 2つのblockを1つのbranchにあわせる
    il_node::arg_array* array = new il_node::arg_array;
    array->array.push_back(mainblock);
    array->array.push_back(resumeblock);
    // OUTPUT: [branch]
    il_node::il_operator* op = new il_node::il_operator("p_branch", array);
    rule->push_back(op);
  }
}

// ファンクタのポインタの先を比較 (<)
struct p_arg_functor_greater
{
  bool operator()(il_node::arg_functor* lhs, il_node::arg_functor* rhs)
  {
    if(lhs->arity < rhs->arity){
      return true;
    }else if(lhs->arity == rhs->arity){
      return lhs->name < rhs->name;
    }else{
      return false;
    }
  }
};

static void pickup_functors(std::set<il_node::arg_functor*,p_arg_functor_greater>& funcs, std::list<il_node::il_operator*>& block);

static void pickup_functors(std::set<il_node::arg_functor*,p_arg_functor_greater>& funcs, il_node::arg_base* arg)
{
  if(il_node::arg_block* block = dynamic_cast<il_node::arg_block*>(arg)){
    // 入れ子なら再帰
    pickup_functors(funcs, block->block);
  }else if(il_node::arg_array* array = dynamic_cast<il_node::arg_array*>(arg)){
    for(unsigned int i=0; i<array->array.size(); ++i){
      // 可変長引数でも再帰
      pickup_functors(funcs, array->array[i]);
    }
  }else if(il_node::arg_functor* func = dynamic_cast<il_node::arg_functor*>(arg)){
    // ファンクタなら追加する
    funcs.insert(func);
  }else{
    // それ以外なら放置
  }
}

static void pickup_functors(std::set<il_node::arg_functor*,p_arg_functor_greater>& funcs, std::list<il_node::il_operator*>& block)
{
  for(std::list<il_node::il_operator*>::iterator it = block.begin();
      it != block.end();
      ++it){
    for(unsigned int j=0; j<(*it)->arguments.size(); ++j){
      pickup_functors(funcs, (*it)->arguments[j]);
    }
  }
  
}

// どこかで出てくるファンクタをfuncsに追加する
static void pickup_functors(std::set<il_node::arg_functor*,p_arg_functor_greater>& funcs, std::vector<std::list<il_node::il_operator*>*>& blocks)
{
  for(unsigned int i=0; i<blocks.size(); ++i){
    pickup_functors(funcs, *blocks[i]);
  }
}

// ルールが既にあるファンクタをfuncsから消す
static void erase_ruled_functors(std::set<il_node::arg_functor*,p_arg_functor_greater>& funcs, std::vector<std::list<il_node::il_operator*>*>& blocks)
{
  for(unsigned int i=0; i<blocks.size(); ++i){
    // 先頭は[function]のはず
    assert(blocks[i]->front()->opecode == "function");
    il_node::arg_functor* func = dynamic_cast<il_node::arg_functor*>(blocks[i]->front()->arguments[0]);
    // すでに拾っているはずなので単に消せばよい
    funcs.erase(func);
  }
}

// ルールのないファンクタを扱うためのルールを追加する (将来的にはリンカの仕事)
static void add_rules_for_unruled_functors(std::set<il_node::arg_functor*,p_arg_functor_greater>& funcs,
                                           std::vector<std::list<il_node::il_operator*>*>& blocks)
{
  for(std::set<il_node::arg_functor*,p_arg_functor_greater>::iterator it = funcs.begin();
      it != funcs.end();
      ++it){
    // 1つルールを追加し,その中にアトム生成処理を追加する このアトムはルール試行を行う必要は無い
    // TODO: 分割コンパイルを考えるとこの処理は後々変える必要がある
    blocks.push_back(new std::list<il_node::il_operator*>());
    add_first_atom_create_operators(blocks.back(), *it, false);
    blocks.back()->push_front(new il_node::il_operator("function", new il_node::arg_functor((*it)->name, (*it)->arity)));
  }
}

// blocksを関数として使える形に整形する
static void optimize_functionalize(il_node::il_root* root)
{
  // ルール
  std::vector<std::list<il_node::il_operator*>*>& blocks = root->rules;
  
  // 0番(最初にfindatomで拾うアトム)を消す命令を用意しておく
  il_node::il_operator* free_firstatom_op = new il_node::il_operator("freeatom", new il_node::arg_index(0));
  // 0番(最初にfindatomで拾うアトム)との比較命令を用意しておく(0番が先)
  il_node::il_operator* neq_firstatom_op1 = new il_node::il_operator("p_neqatom", new il_node::arg_index(0), new il_node::arg_wildcard());
  // 0番(最初にfindatomで拾うアトム)との比較命令を用意しておく(0番が後)
  il_node::il_operator* neq_firstatom_op2 = new il_node::il_operator("p_neqatom", new il_node::arg_wildcard(), new il_node::arg_index(0));
  
  // TODO:他の最適化の後なので,本当はこの時点ではもう完全入れ子を期待できない
  for(unsigned int i=0; i<blocks.size(); ++i){
    // 一番最初のfindatomを外す
    il_node::il_operator* first_findatom = blocks[i]->front();
    blocks[i]->pop_front();
    // findatomの入れ子(3番目)部分をブロックの先頭側に突っ込む
    il_node::arg_block* inner = dynamic_cast<il_node::arg_block*>(first_findatom->arguments[2]);
    blocks[i]->splice(blocks[i]->begin(), inner->block);
    // findatomのファンクタを外す
    il_node::arg_functor* func = dynamic_cast<il_node::arg_functor*>(first_findatom->arguments[1]);
    first_findatom->arguments[1] = 0;
    delete first_findatom;

    // 最初のfindatomしたアトムからのgetlinkを消す(というか頭からarity個分命令を消せばよいだけ)
    for(int j=0; j<func->arity; ++j){
      il_node::il_operator* op = blocks[i]->front();
      blocks[i]->pop_front();
      assert(op->opecode == "getlink");
      delete op;
    }

    // 主導アトムとのneqatomを消す
    delete_operator(blocks[i], neq_firstatom_op1);
    delete_operator(blocks[i], neq_firstatom_op2);

    // 最初のfindatomで拾うアトムに対するfreeatomを消す
    delete_operator(blocks[i], free_firstatom_op);

    // 失敗時再生用のbranch入れる (TODO: ここでやらなきゃいけない処理なのか？)
    add_resume_branch(blocks[i], func);
    
    // 関数のシグネチャとなる目印代わりの命令を突っ込む
    // OUTPUT: [function]
    blocks[i]->push_front(new il_node::il_operator("function", func));
  }
  
  // 使われているのにルールのないファンクタ用のルールを用意する ([call]を[newatom]にインライン化してもいいけど...)
  // キューへのプッシュの有無を考える場合も,ここでデータアトムを確定できれば非常に大きい
  // TODO: 色々 + 整数演算とかも用意する必要がある
  std::set<il_node::arg_functor*,p_arg_functor_greater> functor_occur;
  pickup_functors(functor_occur, blocks);
  pickup_functors(functor_occur, root->initial);
  erase_ruled_functors(functor_occur, blocks);
  add_rules_for_unruled_functors(functor_occur, blocks);

  delete free_firstatom_op;
  delete neq_firstatom_op1;
  delete neq_firstatom_op2;
}

static void optimize_recursive_call(il_operator_list& block, il_node::arg_functor* func);

// 再帰するだけ
static void optimize_recursive_call(il_node::arg_base* arg, il_node::arg_functor* func)
{
  if(il_node::arg_block* block = dynamic_cast<il_node::arg_block*>(arg)){
    optimize_recursive_call(block->block, func);
  }else if(il_node::arg_array* array = dynamic_cast<il_node::arg_array*>(arg)){
    for(unsigned int i=0; i<array->array.size(); ++i){
      optimize_recursive_call(array->array[i], func);
    }
  }
  // その他の引数には何もしないでよい
}

static void optimize_recursive_call(il_operator_list& block, il_node::arg_functor* func)
{
  for(il_operator_list::iterator it = block.begin();
      it != block.end();
      ++it){
    il_node::il_operator* op = *it;
    
    if(op->opecode == "call"){
      if(func->is_same_shallow(op->arguments[0])){
        // もし自分と同じファンクタを呼ぶ[call]なら
        il_operator_list::iterator next_it = it;
        ++next_it;
        if(next_it != block.end()){
          il_node::il_operator* next_op = *next_it;
          if(next_op->opecode == "success"){
            // もしcallの次にsuccess命令が入っていたら[call]はループ化しても大丈夫
            // OUTPUT: [recall] 引数はそのまま
            op->opecode = "recall";
          }
        }
      }
    }else{
      // [call]以外なら引数の中をチェック
      for(unsigned int i=0; i<op->arguments.size(); ++i){
        optimize_recursive_call(op->arguments[i], func);
      }
    }
  }
}

static void optimize_recursive_call(std::vector<il_operator_list*>& rules)
{
  for(unsigned int i=0; i<rules.size(); ++i){
    // ちゃんと先頭が[function]になっているか一応確認
    assert(rules[i]->front()->opecode == "function");
    // そのルールの主導アトムを取得
    il_node::arg_functor* f = dynamic_cast<il_node::arg_functor*>(rules[i]->front()->arguments[0]);
    optimize_recursive_call(*rules[i], f);
  }
}

void optimizer::optimize(il_node::il_root* root)
{
  // リンク再利用
  // TODO:多分アトム再利用とかはこっちでやる(branchが無い状態でしたい)
  optimize_reusing(root->rules);
  
  // 編み上げをする 少なくとも1段目までは必須
  optimize_merging(root->rules);

  // TODO:多分グループ化とかはここでやる 編み上げの前にすると入れ子が複雑になる

  // 関数の形になるようにいろいろする
  optimize_functionalize(root);

  // 再帰呼び出しの最適化
  optimize_recursive_call(root->rules);
}

