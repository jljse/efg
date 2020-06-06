#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <string>
#include <vector>
#include "location.hh"
#include "p_node.h"

class parser {
public:
  // ファイルのパージングを行う, パージングが成功した場合パーズツリーを,失敗した場合NULLを返す
  p_node::p_root* parse(const std::string& filename);
  
  // 内部でパーズ中エラーが起きたときに使う
  void error(const yy::location& loc, std::string s);
  // 内部でパーズ中エラーが起きたときに使う
  void error(std::string s);
  // 内部でパーズが終了したときに使う
  void set_result_yy(p_node::p_root*);
  // 内部でparse後,パージング結果を取得する
  p_node::p_root* get_result_yy();
private:
  p_node::p_root* result_yy;
  void init_yy(const std::string& name);
  void finit_yy();
};

#endif
