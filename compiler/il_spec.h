#ifndef IL_SPEC_H_INCLUDED
#define IL_SPEC_H_INCLUDED

#include <string>

// 命令の引数の種類 indexについては,中身と操作によってわかれている
// 引数のindexの先にあるリンクを使用するならUSE_LINK,
// 引数のindexの先にあるアトムを使用するならUSE_ATOM,
// 引数のindexの先にある整数を定義するならSET_INT // TODO:この辺の整数の扱いはもっと考える必要がある
// INTは整数即値
// BLOCKは入れ子の命令列,
// BLOCKSは命令列が複数個入ったもの,
// LINKSはリンクを示すindexが複数個入ったもの
// PCTYPEはプロセス文脈の種類
// ENDは引数列の終わりを表す
enum SPEC_ARG {
  SPEC_ARG_END,
  SPEC_ARG_USE_LINK,
  SPEC_ARG_USE_ATOM,
  SPEC_ARG_USE_INT,
  SPEC_ARG_SET_LINK,
  SPEC_ARG_SET_ATOM,
  SPEC_ARG_SET_INT,
  SPEC_ARG_INT,
  SPEC_ARG_FUNCTOR,
  SPEC_ARG_BLOCK,
  SPEC_ARG_LINKS,
  SPEC_ARG_BLOCKS,
  SPEC_ARG_PCTYPE,
};

// 中間命令の整数表現 (今は使っていない)
enum INSTR {
  INSTR_NOTBUDDY,
  INSTR_GETBUDDY,
  INSTR_DEREF,
  INSTR_GETLINK,
  INSTR_ISBUDDY,
  INSTR_FINDATOM,
  INSTR_NEQATOM,
  INSTR_FREELINK,
  INSTR_FREEATOM,
  INSTR_UNCONNECT,
  INSTR_NEWLINK,
  INSTR_BEBUDDY,
  INSTR_UNIFY,
  INSTR_CALL,
  INSTR_COMMIT,
  INSTR_SUCCESS,
  INSTR_BRANCH,
  INSTR_NEWATOM,
  INSTR_SETLINK,
  INSTR_FAILURE,
  INSTR_FUNCTION,
  INSTR_ENQUEUE,
  INSTR_RECALL,
  INSTR_NEWINT,
  INSTR_DEREFPC,
  INSTR_COPYPC,
  INSTR_MOVEPC,
  INSTR_FREEPC,
};

// 中間命令の仕様
struct il_spec
{
  const char* str;
  const enum INSTR opecode;
  const enum SPEC_ARG arg[16]; // 引数領域を適当に16個(.cppの記述性のため) 一応0終端だがSPEC_ARG_ENDと比較すること
};

// 命令の文字列表現からil_specを取得
const il_spec* str_to_spec(const std::string& str);

// 整数表現からil_specを取得
const il_spec* enum_to_spec(enum INSTR n);

// @はindex, #は即値, $はファンクタ, {}は入れ子の命令列, ()は不定長引数
// @a はatom入り, @l はリンク入り, @iは整数入り,
// @A はそこでアトムを作る, @L はそこでリンクを作る, @Iはそこで整数を作る
// p_がつく命令は失敗の可能性がある
// 一見SSAっぽいけどbebuddyとかunifyとかいろいろ破綻してる気がする

// [p_notbuddy @l @l] 1と2がbuddyでない
// [getbuddy @L @l] 1に2のbuddyを代入
// [p_deref @A @l # $] 2の先のアトムが4で3番目につながっていることを確認したあとアトムを1に代入
// [getlink @L @a #] 1に2の3番目のリンクを代入
// [p_isbuddy @l @l] 1と2がbuddyである
// [p_findatom @A $ {}] 1に2のアトムを代入,3の実行 を繰り返す
// [p_neqatom @a @a] 1と2が異なるアトムである
// [freelink @l] 1のリンクを消す
// [freeatom @a] 1のアトムを消す
// [unconnect @L @l] 2のリンクのアトム情報を消し,1に代入 (SSAにしようとした痕跡)
// [newlink @L] 1にリンクを生成
// [bebuddy @l @l] 1と2をbuddyにする (SSAなら直さないと)
// [unify @l @l] 1のbuddyと2のbuddyをbuddyにし,1と2は消す (=[getbuddy p 1],[getbuddy q 2],[bebuddy p q],[freelink 1],[freelink 2])
// [call $ (@l)] 1の関数を2を引数として呼び出す
// [commit] ルールマッチング成功の目印
// [success] ルール成功終了
// [p_branch ({})] 1の中で分岐し,どれかが成功するまで試していく
// [newatom @A $] 1に2のアトムを生成
// [setlink @l @a #] 1のリンクを2のアトムの3番目に接続 (SSAなら直さないと)
// [failure] ルール失敗終了 (branchの中で出てきた場合の振る舞いはまだ考えてない)
// [function $] 1のためのルール開始であることの目印
// [enqueue @a] 1を後で再実行する
// [recall $ (@l)] 自分を呼び出すが,ループ化してよい命令 functorは必ずそのルールの主導アトムと一致する
// [newint @l #] 1に2の整数アトムを生成してつなぐ
// [p_derefpc @A @l ?] 2の先のプロセス文脈が3にマッチしていることを確認して1に代入
// [copypc @l @a ?] 1に2のプロセス文脈(種類は3)をコピーしてつなぐ
// [movepc @l @a ?] 1に2のプロセス文脈(種類は3)を引き継いでつなぐ
// [freepc @a ?] 1のプロセス文脈(種類は3)を削除
#endif

