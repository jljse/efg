#include "generator.h"
#include "il_node.h"
#include <utility>
#include <sstream>
#include <cassert>
#include <map>
#include "il_spec.h"
#include "utility.h"

// シンボル,ファンクタを管理するテーブル
class literal_table
{
public:
  std::vector<std::string> symbols;
  std::vector<std::pair<int,int> > functors;

  // シンボルを一意な整数表現にする
  int symbol_to_int(const std::string& name);
  // ファンクタを一意な整数表現にする
  int functor_to_int(const il_node::arg_functor*);
};

int literal_table::symbol_to_int(const std::string& name)
{
  for(unsigned int i=0; i<symbols.size(); ++i){
    if(name == symbols[i]){
      return i;
    }
  }

  symbols.push_back(name);
  return symbols.size() - 1;
}

int literal_table::functor_to_int(const il_node::arg_functor* func)
{
  int symid = symbol_to_int(func->name);
  
  for(unsigned int i=0; i<functors.size(); ++i){
    if(symid==functors[i].first && func->arity==functors[i].second){
      return i;
    }
  }

  functors.push_back(std::make_pair(symid,func->arity));
  return functors.size() - 1;
}

// 中間変数の種類
enum VARS_TYPE {
  VARS_TYPE_UNKNOWN,
  VARS_TYPE_LINK,
  VARS_TYPE_ATOM,
};

// 中間変数の情報
struct vars_type
{
  bool already_declared; // その変数が親によって宣言されているか
  enum VARS_TYPE type; // その変数の種類
  int functor_type; // typeがatomの場合のみ使う(-1ならunknownということに)

  vars_type() { already_declared=false; type=VARS_TYPE_UNKNOWN; functor_type=-1; }
};

// 変数情報 ブロックにつき1つ対応する感じで
class vars_type_context
{
  literal_table& lt;
public:
  std::vector<vars_type> data;
  
  // parentを親ブロックとし,その変数の環境を(一部)引き継ぐ
  void inherit_vars(const vars_type_context& parent);
  // このブロックで,あるindexの変数をリンクとして代入する
  void set_link(il_node::arg_index*);
  // このブロックで,あるindexの変数をアトムとして代入する
  void set_atom(il_node::arg_index*, il_node::arg_functor*);
  // このブロックで,あるindexの変数をリンクとして使用する
  void use_link(il_node::arg_index*);
  // このブロックで,あるindexの変数をアトムとして使用する
  void use_atom(il_node::arg_index*);

  // 実際問題 set_atom(index,func) と set_atom(index) と set_link(index) だけあればよかった ほとんど同じ関数が並んで気持ち悪い

  // ファンクタの変換に使用するテーブルを最初に登録する
  vars_type_context(literal_table& t) : lt(t) {}
};

void vars_type_context::inherit_vars(const vars_type_context& parent)
{
  data.resize(parent.data.size());

  for(unsigned int i=0; i<parent.data.size(); ++i){
    // あるindexの変数が何らかの型で宣言されていたら,それを継承する
    if(parent.data[i].type != VARS_TYPE_UNKNOWN){
      data[i].already_declared = true;
      data[i].type = parent.data[i].type;
      data[i].functor_type = parent.data[i].type;
    }
  }
}

void vars_type_context::set_link(il_node::arg_index* arg)
{
  if(data.size() <= static_cast<unsigned int>(arg->index)) data.resize(arg->index + 1);
  
  int index = arg->index;
  
  if(data[index].already_declared){
    // 外で宣言済みの変数の場合, 型が違っていても再宣言すればOK
    if(data[index].type != VARS_TYPE_LINK){
      data[index].already_declared = false;
      data[index].type = VARS_TYPE_LINK;
      data[index].functor_type = -1;
    }
    // 一致しているならそのまま使うので何もしないでよい
  }else{
    // このブロックで宣言した変数の場合, 型の食い違いは困る
    if(data[index].type == VARS_TYPE_LINK){
      // 一致していれば文句なし
    }else if(data[index].type == VARS_TYPE_UNKNOWN){
      // 未使用なら問題なし
      data[index].type = VARS_TYPE_LINK;
      data[index].functor_type = -1;
    }else{
      // それ以外の場合型が不一致である
      // 正しく命令が吐けていればそんなことは無いはずだが
      assert(0);
    }
  }
}

void vars_type_context::set_atom(il_node::arg_index* arg, il_node::arg_functor* func)
{
  if(data.size() <= static_cast<unsigned int>(arg->index)) data.resize(arg->index + 1);

  int index = arg->index;
  
  if(data[index].already_declared){
    // 外で宣言済みの変数の場合, 型が違っていても再宣言すればOK (ファンクタが違う場合再宣言してもよいがまぁそのまま)
    if(data[index].type != VARS_TYPE_ATOM){
      data[index].already_declared = false;
      data[index].type = VARS_TYPE_ATOM;
      data[index].functor_type = lt.functor_to_int(func);
    }
    // 一致しているならそのまま使うので何もしないでよい
  }else{
    // このブロックで宣言した変数の場合, 型の食い違いは困る
    if(data[index].type == VARS_TYPE_ATOM){
      if(data[index].functor_type == lt.functor_to_int(func)){
        // 完全に一致していれば文句なし
      }else{
        // ファンクタが違うのも困る
        assert(0);
      }
    }else if(data[index].type == VARS_TYPE_UNKNOWN){
      // 未使用なら問題なし
      data[index].type = VARS_TYPE_ATOM;
      data[index].functor_type = lt.functor_to_int(func);
    }else{
      // それ以外の場合型が不一致である
      // 正しく命令が吐けていればそんなことは無いはずだが
      assert(0);
    }
  }
}

void vars_type_context::use_link(il_node::arg_index* arg)
{
  if(data.size() <= static_cast<unsigned int>(arg->index)) data.resize(arg->index + 1);

  int index = arg->index;
  
  if(data[index].already_declared){
    // 外で宣言済みの変数の場合, 型が違っていても再宣言すればOK
    if(data[index].type != VARS_TYPE_LINK){
      data[index].already_declared = false;
      data[index].type = VARS_TYPE_LINK;
      data[index].functor_type = -1;
    }
    // 一致しているならそのまま使うので何もしないでよい
  }else{
    // このブロックで宣言した変数の場合, 型の食い違いは困る
    if(data[index].type == VARS_TYPE_LINK){
      // 一致していれば文句なし
    }else if(data[index].type == VARS_TYPE_UNKNOWN){
      // 未使用なら問題なし
      data[index].type = VARS_TYPE_LINK;
    }else{
      // それ以外の場合型が不一致である
      // 正しく命令が吐けていればそんなことは無いはずだが
      assert(0);
    }
  }
}

void vars_type_context::use_atom(il_node::arg_index* arg)
{
  if(data.size() <= static_cast<unsigned int>(arg->index)) data.resize(arg->index + 1);

  int index = arg->index;
  
  if(data[index].already_declared){
    // 外で宣言済みの変数の場合, 型が違っていても再宣言すればOK
    if(data[index].type != VARS_TYPE_ATOM){
      data[index].already_declared = false;
      data[index].type = VARS_TYPE_ATOM;
      data[index].functor_type = -1;
    }
    // 一致しているならそのまま使うので何もしないでよい
  }else{
    // このブロックで宣言した変数の場合, 型の食い違いは困る
    if(data[index].type == VARS_TYPE_ATOM){
      // 一致していれば文句なし 使うだけならファンクタへの制限は不明なのでファンクタは無視
    }else if(data[index].type == VARS_TYPE_UNKNOWN){
      // 未使用なら問題なし
      data[index].type = VARS_TYPE_ATOM;
      data[index].functor_type = -1;
    }else{
      // それ以外の場合型が不一致である
      // 正しく命令が吐けていればそんなことは無いはずだが
      assert(0);
    }
  }
}

// 文字列を, ""でくくられた文字列リテラルとして適当な形にする(""自体は含めない)
static void escape_string_literal(std::string& s)
{
  std::stringstream ss;
  for(unsigned int i=0; i<s.size(); ++i){
    switch(s[i]){
    case '\n':
      // 改行が含まれる場合エスケープした文字列を入れる
      ss << "\\n";
      break;
    case '"':
      // "が含まれる場合エスケープした文字列を入れる
      ss << "\\\"";
      break;
    default:
      // 普通はそのまま
      ss << s[i];
      break;
    }
  }
  // 元の場所に戻す
  ss.str().swap(s);
}

static std::string functor_to_signature_naive(const std::string& name, int arity)
{
  std::stringstream ss;
  
  // + - * / . [] だけは例外 これらは適当な名前に変える
  if(name == "+"){
    ss << "gr__plus";
  }else if(name == "-"){
    ss << "gr__gminus";
  }else if(name == "*"){
    ss << "gr__mul";
  }else if(name == "/"){
    ss << "gr__div";
  }else if(name == "."){
    ss << "gr__dot";
  }else if(name == "[]"){
    ss << "gr__nil";
  }else{
    ss << "gr_" << name;
  }
  
  ss << "_" << arity;
  return ss.str();
}

// ファンクタを関数シグネチャの形に変換する
static std::string functor_to_signature_naive(const il_node::arg_functor* func)
{
  return functor_to_signature_naive(func->name, func->arity);
}

std::string index_to_varname(const il_node::arg_index* i)
{
  std::stringstream ss;
  ss << "x" << i->index;
  return ss.str();
}

std::string index_to_varname(int i)
{
  std::stringstream ss;
  ss << "x" << i;
  return ss.str();
}

// 再帰時に引数を再設定する際の代入の順番を設定して出力する
// A->B; C->D; ... と要求がある場合, 各変数は右辺より後に左辺に出てきてはいけない
// 今回の場合
//   ある式の左辺が配置済みの式の右辺に出現している -> その直前に置く
//   ある式の左辺が配置済みの式の右辺に出現していない -> 最後尾に置く
// という処理を1通りするだけでよい ... はず
static void output_vars_substitution(std::ostream& out,
                                     int indent,
                                     il_node::arg_array* array)
{
  std::list<std::pair<int,int> > substitutions;
  
  for(unsigned int i=0; i<array->array.size(); ++i){
    int x = dynamic_cast<il_node::arg_index*>(array->array[i])->index;
    // 既にsubstitutionsの中に配置されたか
    bool inserted_flag = false;
    
    for(std::list<std::pair<int,int> >::iterator it = substitutions.begin();
        it != substitutions.end();
        ++it){
      if(it->second == x){
        // 右辺に発見したらその直前に挿入 (x0が主導アトムなので i番目の引数はx(i+1)への代入 )
        substitutions.insert(it, std::make_pair(i+1, x));
        inserted_flag = true;
        break;
      }
    }
    
    // 右辺に出現していなければ最後に挿入 (同上)
    if(! inserted_flag){
      substitutions.push_back(std::make_pair(i+1, x));
    }
  }
  
  // 全部配置し終わったら出力
  for(std::list<std::pair<int,int> >::iterator it = substitutions.begin();
      it != substitutions.end();
      ++it){
    out << ntimes_space(indent) << index_to_varname(it->first) << " = " << index_to_varname(it->second) << ";" << std::endl;
  }
}

static void generate_block(const std::list<il_node::il_operator*>& block,
                           literal_table& lt,
                           std::ostream& out,
                           int indent,
                           vars_type_context& parent_vtc,
                           const std::string& success_code,
                           const std::string& failure_code);

static void generate_inner_block(const std::list<il_node::il_operator*>& block,
                                 literal_table& lt,
                                 std::ostream& out,
                                 int indent,
                                 vars_type_context& vtc,
                                 const std::string& success_code,
                                 const std::string& failure_code);
// 命令をコードとして出力
static void generate_operator(const il_node::il_operator* op,
                              literal_table& lt,
                              std::ostream& out,
                              int indent,
                              vars_type_context& vtc,
                              const std::string& success_code,
                              const std::string& failure_code)
{
  // 変にオブジェクト指向を狙わない方がいい
  if(op->opecode == "p_notbuddy"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    il_node::arg_index* a1 = dynamic_cast<il_node::arg_index*>(op->arguments[1]);
    std::string s0 = index_to_varname(a0);
    std::string s1 = index_to_varname(a1);
    out << ntimes_space(indent) << "if( " << s0 << "->buddy == " << s1 << " ){ " << failure_code << " }" << std::endl;
  }else if(op->opecode == "getbuddy"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    il_node::arg_index* a1 = dynamic_cast<il_node::arg_index*>(op->arguments[1]);
    std::string s0 = index_to_varname(a0);
    std::string s1 = index_to_varname(a1);
    out << ntimes_space(indent) << s0 << " = " << s1 << "->buddy;" << std::endl;
  }else if(op->opecode == "p_deref"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    il_node::arg_index* a1 = dynamic_cast<il_node::arg_index*>(op->arguments[1]);
    il_node::arg_integer* a2 = dynamic_cast<il_node::arg_integer*>(op->arguments[2]);
    il_node::arg_functor* a3 = dynamic_cast<il_node::arg_functor*>(op->arguments[3]);
    std::string s0 = index_to_varname(a0);
    std::string s1 = index_to_varname(a1);
    int s2 = a2->data;
    int s3 = lt.functor_to_int(a3);
    out << ntimes_space(indent) << "if(" << s1 << "->pos != " << s2 << "){ " << failure_code << " }" << std::endl;
    out << ntimes_space(indent) << "if(" << s1 << "->atom->functor != " << s3 << "){ " << failure_code << " }" << std::endl;
    out << ntimes_space(indent) << s0 << " = " << s1 << "->atom;" << std::endl;
  }else if(op->opecode == "getlink"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    il_node::arg_index* a1 = dynamic_cast<il_node::arg_index*>(op->arguments[1]);
    il_node::arg_integer* a2 = dynamic_cast<il_node::arg_integer*>(op->arguments[2]);
    std::string s0 = index_to_varname(a0);
    std::string s1 = index_to_varname(a1);
    int s2 = a2->data;
    out << ntimes_space(indent) << s0 << " = g_getlink(" << s1 << "," << s2 << ");" << std::endl;
  }else if(op->opecode == "p_isbuddy"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    il_node::arg_index* a1 = dynamic_cast<il_node::arg_index*>(op->arguments[1]);
    std::string s0 = index_to_varname(a0);
    std::string s1 = index_to_varname(a1);
    out << ntimes_space(indent) << "if( " << s0 << "->buddy != " << s1 << " ){ " << failure_code << " }" << std::endl;
  }else if(op->opecode == "p_findatom"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    il_node::arg_functor* a1 = dynamic_cast<il_node::arg_functor*>(op->arguments[1]);
    il_node::arg_block* a2 = dynamic_cast<il_node::arg_block*>(op->arguments[2]);
    std::string s0 = index_to_varname(a0);
    int s1 = lt.functor_to_int(a1);
    out << ntimes_space(indent) << "for(" << s0 << " = g_atomlist_begin(th, " << s1 << ");" << std::endl;
    out << ntimes_space(indent) << "    " << s0 << " != g_atomlist_end(th, " << s1 << ");" << std::endl;
    out << ntimes_space(indent) << "    " << s0 << " = g_atomlist_next(" << s0 << ")){" << std::endl;
    generate_inner_block(a2->block, lt, out, indent+1, vtc, success_code, "continue;");
    out << ntimes_space(indent) << "}" << std::endl;
    // 普通にループを抜けてしまった場合は失敗
    out << ntimes_space(indent) << failure_code << std::endl;
    // TODO:成功した場合findatomから抜けて次にいくようにする場合はここにラベルを作る
  }else if(op->opecode == "p_neqatom"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    il_node::arg_index* a1 = dynamic_cast<il_node::arg_index*>(op->arguments[1]);
    std::string s0 = index_to_varname(a0);
    std::string s1 = index_to_varname(a1);
    out << ntimes_space(indent) << "if( " << s0 << " == " << s1 << " ){ " << failure_code << " }" << std::endl;
  }else if(op->opecode == "freelink"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    std::string s0 = index_to_varname(a0);
    out << ntimes_space(indent) << "g_freelink(th, " << s0 << ");" << std::endl;
  }else if(op->opecode == "freeatom"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    std::string s0 = index_to_varname(a0);
    out << ntimes_space(indent) << "g_freeatom(th, " << s0 << ");" << std::endl; //TODO: ここでファンクタも出せるけど...
  }else if(op->opecode == "unconnect"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    il_node::arg_index* a1 = dynamic_cast<il_node::arg_index*>(op->arguments[1]);
    std::string s0 = index_to_varname(a0);
    std::string s1 = index_to_varname(a1);
    out << ntimes_space(indent) << s0 << " = " << s1 << ";  " << s0 << "->pos = -1;" << std::endl; //未接続の条件はpos==-1 atomは関係ない
  }else if(op->opecode == "newlink"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    std::string s0 = index_to_varname(a0);
    out << ntimes_space(indent) << s0 << " = g_newlink(th);" << std::endl;
  }else if(op->opecode == "bebuddy"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    il_node::arg_index* a1 = dynamic_cast<il_node::arg_index*>(op->arguments[1]);
    std::string s0 = index_to_varname(a0);
    std::string s1 = index_to_varname(a1);
    out << ntimes_space(indent) << s0 << "->buddy = " << s1 << ";  " << s1 << "->buddy = " << s0 << ";" << std::endl;
  }else if(op->opecode == "unify"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    il_node::arg_index* a1 = dynamic_cast<il_node::arg_index*>(op->arguments[1]);
    std::string s0 = index_to_varname(a0);
    std::string s1 = index_to_varname(a1);
    out << ntimes_space(indent) << s0 << "->buddy->buddy = " << s1 << "->buddy;" << std::endl;
    out << ntimes_space(indent) << s1 << "->buddy->buddy = " << s0 << "->buddy;" << std::endl;
    out << ntimes_space(indent) << "g_freelink(th, " << s0 << ");" << std::endl;
    out << ntimes_space(indent) << "g_freelink(th, " << s1 << ");" << std::endl;
  }else if(op->opecode == "call"){
    il_node::arg_functor* a0 = dynamic_cast<il_node::arg_functor*>(op->arguments[0]);
    il_node::arg_array* a1 = dynamic_cast<il_node::arg_array*>(op->arguments[1]);
    std::string s0 = functor_to_signature_naive(a0);
    out << ntimes_space(indent) << s0 << "(";
    out << "th";
    for(unsigned int i=0; i<a1->array.size(); ++i){
      out << ",";
      il_node::arg_index* x = dynamic_cast<il_node::arg_index*>(a1->array[i]);
      std::string s = index_to_varname(x);
      out << s;
    }
    out << ");" << std::endl;
  }else if(op->opecode == "recall"){
    // この実装では不要 il_node::arg_functor* a0 = dynamic_cast<il_node::arg_functor*>(op->arguments[0]);
    il_node::arg_array* a1 = dynamic_cast<il_node::arg_array*>(op->arguments[1]);
    // 適切な順番で代入を行う必要がある
    output_vars_substitution(out, indent, a1);
    // TODO: label_loopという名前がいきなり出てくるのはどうか
    out << ntimes_space(indent) << "goto label_loop;" << std::endl;
  }else if(op->opecode == "commit"){
    // TODO: 適当にルールの情報を出す
    out << ntimes_space(indent) << "g_commit();" << std::endl;
  }else if(op->opecode == "success"){
    out << ntimes_space(indent) << success_code << std::endl;
  }else if(op->opecode == "p_branch"){
    il_node::arg_array* array = dynamic_cast<il_node::arg_array*>(op->arguments[0]);
    assert(array);

    // ジャンプのためのラベル名を生成 一意なら何でもいい
    const void* op_ptr = static_cast<const void*>(op);
    std::stringstream ss;
    ss << "label_" << op_ptr << "_";
    std::string label_name = ss.str();

    for(unsigned int i=0; i<array->array.size(); ++i){
      // ブロックを開く
      out << ntimes_space(indent) << "{" << std::endl;
      
      il_node::arg_block* block = dynamic_cast<il_node::arg_block*>(array->array[i]);
      assert(block);

      // 失敗時実行すべきコードを生成
      std::stringstream ss;
      ss << "goto " << label_name << i << ";";
      // 中身のブロックを出力 このブロック内での失敗は失敗時用ラベルへのgotoにする
      generate_block(block->block, lt, out, indent+1, vtc, success_code, ss.str());
      
      // ブロックを閉じて
      out << ntimes_space(indent) << "}" << std::endl;
      // 失敗時用のラベルを出力
      out << ntimes_space(indent-1) << label_name << i << ":" << std::endl;
    }
    // 全枝で失敗した場合全体が失敗
    out << ntimes_space(indent) << failure_code << std::endl;
    // branchから合流することを考える場合ここに成功時ラベルを用意する
  }else if(op->opecode == "newatom"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    il_node::arg_functor* a1 = dynamic_cast<il_node::arg_functor*>(op->arguments[1]);
    std::string s0 = index_to_varname(a0);
    int s1 = lt.functor_to_int(a1);
    out << ntimes_space(indent) << s0 << " = g_newatom(th, " << s1 << ");" << std::endl; // TODO: サイズも分かってるのでもっと突っ込んでもいい
  }else if(op->opecode == "setlink"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    il_node::arg_index* a1 = dynamic_cast<il_node::arg_index*>(op->arguments[1]);
    il_node::arg_integer* a2 = dynamic_cast<il_node::arg_integer*>(op->arguments[2]);
    std::string s0 = index_to_varname(a0);
    std::string s1 = index_to_varname(a1);
    int s2 = a2->data;
    out << ntimes_space(indent) << "g_setlink(" << s1 << "," << s2 << "," << s0 << ");" << std::endl;
  }else if(op->opecode == "failure"){
    out << ntimes_space(indent) << failure_code << std::endl;
  }else if(op->opecode == "function"){
    // 何もしない
  }else if(op->opecode == "enqueue"){
    il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    std::string s0 = index_to_varname(a0);
    out << ntimes_space(indent) << "g_enqueue(th, " << s0 << ");" << std::endl; // TODO: ここでファンクタ出せる
    // TODO: ここに命令を追加
  }else if(op->opecode == "p_derefint"){
    assert(0);
    // il_node::arg_index* a0 = dynamic_cast<il_node::arg_index*>(op->arguments[0]);
    // il_node::arg_index* a1 = dynamic_cast<il_node::arg_index*>(op->arguments[1]);
    // il_node::arg_integer* a2 = dynamic_cast<il_node::arg_integer*>(op->arguments[2]);
    // std::string s0 = index_to_varname(a0);
    // std::string s1 = index_to_varname(a1);
    // int s2 = a2->data;
  //   out << ntimes_space(indent) << "if(" << s1 << "->pos != " << s2 << "){ " << failure_code << " }" << std::endl;
  //   out << ntimes_space(indent) << "if(" << s1 << "->atom->functor != " << s3 << "){ " << failure_code << " }" << std::endl;
  //   out << ntimes_space(indent) << s0 << " = " << s1 << "->atom;" << std::endl;
  }else{
    // 知らない命令
    assert(0);
  }
}

// ブロックをコードとして出力(変数宣言なし)
static void generate_inner_block(const std::list<il_node::il_operator*>& block,
                                 literal_table& lt,
                                 std::ostream& out,
                                 int indent,
                                 vars_type_context& vtc,
                                 const std::string& success_code,
                                 const std::string& failure_code)
{
  for(std::list<il_node::il_operator*>::const_iterator it = block.begin();
      it != block.end();
      ++it){
    generate_operator(*it, lt, out, indent, vtc, success_code, failure_code);
  }
}

static void pickup_vars_type(const std::list<il_node::il_operator*>& block,
                             vars_type_context& vtc);

// opの引数からわかる中間変数型情報をvtcに追加する
static void pickup_vars_type(const il_node::il_operator* op,
                             vars_type_context& vtc)
{
  // branch以外はどんどん掘っていく branchは中の型情報が独立しているので触らない
  if(op->opecode == "branch"){
    return;
  }

  // その命令の型情報を取得
  const il_spec* sp = str_to_spec(op->opecode);
  // 各引数について
  for(int i=0; sp->arg[i]!=SPEC_ARG_END; ++i){
    switch(sp->arg[i]){
    case SPEC_ARG_USE_LINK:
      // 引数はリンクを表している = 引数はindexのはず
      assert(dynamic_cast<il_node::arg_index*>(op->arguments[i]));
      // そのindexをリンクとして使っていることを登録
      vtc.use_link(dynamic_cast<il_node::arg_index*>(op->arguments[i]));
      break;
    case SPEC_ARG_USE_ATOM:
      // 引数はアトムを表わしている = 引数はindexのはず
      assert(dynamic_cast<il_node::arg_index*>(op->arguments[i]));
      // そのindexをアトムとして使っていることを登録
      vtc.use_atom(dynamic_cast<il_node::arg_index*>(op->arguments[i]));
      break;
    case SPEC_ARG_SET_LINK:
      // 引数はリンクを表している = 引数はindexのはず
      assert(dynamic_cast<il_node::arg_index*>(op->arguments[i]));
      // そのindexをリンクとして作成していることを登録
      vtc.set_link(dynamic_cast<il_node::arg_index*>(op->arguments[i]));
      break;
    case SPEC_ARG_SET_ATOM:
      {
        // 無事解析できたかどうか
        bool success_flag = false;
        // 引数はアトムを表している = 引数はindexのはず
        assert(dynamic_cast<il_node::arg_index*>(op->arguments[i]));
        // そのindexをアトムとして作成していることを登録
        // 命令の種類によってどのようなアトムが入るかは違うが, とりあえず同じ命令中にあるファンクタを突っ込むことにする
        for(unsigned int j=0; j<op->arguments.size(); ++j){
          // ファンクタがどこかに入っているはずなのでそれを使う
          if(il_node::arg_functor* func = dynamic_cast<il_node::arg_functor*>(op->arguments[j])){
            vtc.set_atom(dynamic_cast<il_node::arg_index*>(op->arguments[i]), func);
            // 無事発見できた
            success_flag = true;
            break;
          }
        }
        // 何もファンクタがないことはないはずだが一応
        if(! success_flag){
          assert(0);
        }
      }
      break;
    case SPEC_ARG_LINKS:
      // 引数はリンクのみが複数個入っているはず
      {
        il_node::arg_array* array = dynamic_cast<il_node::arg_array*>(op->arguments[i]);
        // arrayになっているか確認
        assert(array);
        for(unsigned int j=0; j<array->array.size(); ++j){
          // 更に中身がindexになっているか確認
          assert(dynamic_cast<il_node::arg_index*>(array->array[j]));
          vtc.use_link(dynamic_cast<il_node::arg_index*>(array->array[j]));
        }
      }
      break;

    // 引数がその他の場合, 中間変数の型の解析の役に立たない

    case SPEC_ARG_BLOCK:
      // 引数はblockのはず
      assert(dynamic_cast<il_node::arg_block*>(op->arguments[i]));
      // blockの場合は再帰的に
      pickup_vars_type(dynamic_cast<il_node::arg_block*>(op->arguments[i])->block, vtc);
      break;

    // それ以外の引数は中間変数型解析については無視してよいはず

    case SPEC_ARG_FUNCTOR:
      assert(dynamic_cast<il_node::arg_functor*>(op->arguments[i]));
      break;
    case SPEC_ARG_INT:
      assert(dynamic_cast<il_node::arg_integer*>(op->arguments[i]));
      break;

    default:
      ;
    }
  }
}

// ブロックから変数の型情報を収集
void pickup_vars_type(const std::list<il_node::il_operator*>& block,
                      vars_type_context& vtc)
{
  for(std::list<il_node::il_operator*>::const_iterator it = block.begin();
      it != block.end();
      ++it){
    // 各命令について処理
    pickup_vars_type(*it, vtc);
  }
}

// 型情報から変数宣言を出力
void generate_vars_declaration(vars_type_context& vtc,
                               literal_table& lt,
                               std::ostream& out,
                               int indent)
{
  bool is_declaration_here = false;
  
  for(unsigned int i=0; i<vtc.data.size(); ++i){
    // 親によって宣言されておらず,自分で宣言する必要がある場合
    if(! vtc.data[i].already_declared){
      switch(vtc.data[i].type){
      case VARS_TYPE_LINK:
        // リンクの場合
        out << ntimes_space(indent) << "struct g_link* " << index_to_varname(i) << ";" << std::endl;
        is_declaration_here = true;
        break;
      case VARS_TYPE_ATOM:
        {
          std::pair<int,int> f = lt.functors[vtc.data[i].functor_type];
          // アトムの場合
          out << ntimes_space(indent) << "struct g_atom* " << index_to_varname(i) << ";  /* " << lt.symbols[f.first] << ":" << f.second << " */" << std::endl;
          is_declaration_here = true;
        }
        break;
      case VARS_TYPE_UNKNOWN:
        // 未知の場合 ... 多分子ブロックで使うのだろうからスルー
        break;
      }
    }
  }
  // 宣言があれば空行を入れる
  if(is_declaration_here) out << std::endl;
}

// ブロックをコードとして出力(変数宣言付き)
static void generate_block(const std::list<il_node::il_operator*>& block,
                           literal_table& lt,
                           std::ostream& out,
                           int indent,
                           vars_type_context& parent_vtc,
                           const std::string& success_code,
                           const std::string& failure_code)
{
  // このブロック用の変数型情報
  vars_type_context vtc(lt);
  
  // 親ブロックから変数の型情報を受け継ぐ
  vtc.inherit_vars(parent_vtc);

  // 変数の型情報を収集する
  pickup_vars_type(block, vtc);
  
  // 変数宣言を出力
  generate_vars_declaration(vtc, lt, out, indent);

  // 処理は丸投げ
  generate_inner_block(block, lt, out, indent, vtc, success_code, failure_code);
}

static void pickup_literal(literal_table& lt, const std::list<il_node::il_operator*>& block);

static void pickup_literal(literal_table& lt, const il_node::arg_base* arg)
{
  if(const il_node::arg_block* block = dynamic_cast<const il_node::arg_block*>(arg)){
    pickup_literal(lt, block->block);
  }else if(const il_node::arg_array* array = dynamic_cast<const il_node::arg_array*>(arg)){
    for(unsigned int i=0; i<array->array.size(); ++i){
      pickup_literal(lt, array->array[i]);
    }
  }else if(const il_node::arg_functor* func = dynamic_cast<const il_node::arg_functor*>(arg)){
    lt.functor_to_int(func);
  }
}

static void pickup_literal(literal_table& lt, const std::list<il_node::il_operator*>& block)
{
  for(std::list<il_node::il_operator*>::const_iterator it = block.begin();
      it != block.end();
      ++it) {
    for(unsigned int i=0; i<(*it)->arguments.size(); ++i){
      pickup_literal(lt, (*it)->arguments[i]);
    }
  }
}

// inに出てくるシンボルとファンクタをltに収集する
static void pickup_literal(literal_table& lt, const il_node::il_root* in)
{
  for(unsigned int i=0; i<in->rules.size(); ++i){
    pickup_literal(lt, *(in->rules[i]));
  }
  pickup_literal(lt, in->initial);
}

// ファンクタや前方宣言等々実行に必要な情報を出力
static void generate_literals(const il_node::il_root* in,
                              literal_table& lt,
                              std::ostream& out)
{
  // 関数の前方宣言を出力
  for(unsigned int i=0; i<in->rules.size(); ++i){
    // 各ルールの最初は[function]のはず
    assert(in->rules[i]->front()->opecode == "function");
    il_node::il_operator* op = in->rules[i]->front();
    il_node::arg_functor* func = dynamic_cast<il_node::arg_functor*>(op->arguments[0]);
    out << "int " << functor_to_signature_naive(func) << "(";
    out << "struct g_thread *th";
    for(int j=0; j<func->arity; ++j){
      out << ", ";
      out << "struct g_link *" << index_to_varname(j+1); // 0は主導アトム, リンクは1から
    }
    out << ");" << std::endl;
  }
  out << std::endl;
  
  // シンボル配列のサイズを出力
  out << "const int g_symbols_size = " << lt.symbols.size() << ";" << std::endl;
  // シンボルを配列として出力
  out << "const char * const g_symbols[" << lt.symbols.size() << "] = {" << std::endl;
  for(unsigned int i=0; i<lt.symbols.size(); ++i){
    if(i != 0) out << "," << std::endl;
    
    std::string s = lt.symbols[i];
    escape_string_literal(s);
    out << "\t\"" << s << "\"";
  }
  out << std::endl << "};" << std::endl << std::endl;

  // ファンクタ配列のサイズを出力
  out << "const int g_functors_size = " << lt.functors.size() << ";" << std::endl;
  // ファンクタを配列として出力
  out << "const struct g_functor g_functors[" << lt.functors.size() << "] = {" << std::endl;
  for(unsigned int i=0; i<lt.functors.size(); ++i){
    if(i != 0) out << "," << std::endl;
    
    out << "\t{" << lt.functors[i].first << "," 
                 << lt.functors[i].second << ","
                 << functor_to_signature_naive(lt.symbols[lt.functors[i].first], lt.functors[i].second) << "}";
  }
  out << std::endl << "};" << std::endl << std::endl;
}

// ルールを関数として出力
static void generate_rule(const std::list<il_node::il_operator*>& rule,
                          literal_table& lt,
                          std::ostream& out)
{
  // 最初の[function]を見て 関数開始 シグネチャ
  assert(rule.front()->opecode == "function");

  il_node::arg_functor* func = dynamic_cast<il_node::arg_functor*>(rule.front()->arguments[0]);
  out << "int " << functor_to_signature_naive(func) << "(";
  out << "struct g_thread *th"; // スレッド情報
  for(int i=0; i<func->arity; ++i){
    out << ", ";
    out << "struct g_link *" << index_to_varname(i+1); // 0は主導アトム, リンクは1から
  }
  out << ")" << std::endl << "{" << std::endl;

  // ルートの型情報
  vars_type_context vtc(lt);
  
  // 主導アトムの引数の情報を入れておく
  // 主導アトムは変数として宣言されているわけではないので入れない
  il_node::arg_index temp(0); // 最悪(^^;
  for(int i=0; i<func->arity; ++i){
    temp.index = i+1;
    // 引数をリンクとしてセット
    vtc.set_link(&temp);
  }
  
  // 再帰のためのラベルを出力 TODO:こんなのいきなり書くのはどうかと
  out << "label_loop:{" << std::endl;
  
  // 中身はgenerate_blockに丸投げ
  generate_block(rule, lt, out, 1, vtc, "return 1;", "return 0;");
  
  // 関数終了
  out << "}/*label_loop*/" << std::endl << "}" << std::endl << std::endl;
}

// 初期構造を関数として出力
static void generate_initial(const std::list<il_node::il_operator*>& rule,
                             literal_table& lt,
                             std::ostream& out)
{
  // main関数?を開始
  out << "void gr__main(struct g_thread *th)" << std::endl << "{" << std::endl;

  // 何も変数は宣言していないので空のまま渡す
  vars_type_context vtc(lt);
  
  // 中身はgenerate_blockに丸投げ
  generate_block(rule, lt, out, 1, vtc, "return 1;", "return 0;");
  
  // 関数終了
  out << "}" << std::endl << std::endl;
}

// 基本となる型などを出力/include文
static void generate_basic(std::ostream& out)
{
  out << "#include \"g_runtime.h\"" << std::endl;
  out << std::endl;
}

void generator::generate(const il_node::il_root* in, std::ostream& out)
{
  literal_table lt;
  // リテラル(シンボルとファンクタ)の情報をltに収集
  pickup_literal(lt, in);

  // 基本となる型などを出力/include文
  generate_basic(out);
  
  // ファンクタとか前方宣言とかもろもろ
  generate_literals(in, lt, out);
  
  for(unsigned int i=0; i<in->rules.size(); ++i){
    // ルール関数の生成
    generate_rule(*in->rules[i], lt, out);
  }
  // 初期データに対応する関数生成
  generate_initial(in->initial, lt, out);
}
