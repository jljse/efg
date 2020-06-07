#include "il_spec.h"

const il_spec il_specs[] = {
  { "p_notbuddy", INSTR_NOTBUDDY,  {SPEC_ARG_USE_LINK,SPEC_ARG_USE_LINK,                                  SPEC_ARG_END} },
  { "getbuddy",   INSTR_GETBUDDY,  {SPEC_ARG_SET_LINK,SPEC_ARG_USE_LINK,                                  SPEC_ARG_END} },
  { "p_deref",    INSTR_DEREF,     {SPEC_ARG_SET_ATOM,SPEC_ARG_USE_LINK,SPEC_ARG_INT,   SPEC_ARG_FUNCTOR, SPEC_ARG_END} },
  { "getlink",    INSTR_GETLINK,   {SPEC_ARG_SET_LINK,SPEC_ARG_USE_ATOM,SPEC_ARG_INT,                     SPEC_ARG_END} },
  { "p_isbuddy",  INSTR_ISBUDDY,   {SPEC_ARG_USE_LINK,SPEC_ARG_USE_LINK,                                  SPEC_ARG_END} },
  { "p_findatom", INSTR_FINDATOM,  {SPEC_ARG_SET_ATOM,SPEC_ARG_FUNCTOR, SPEC_ARG_BLOCK,                   SPEC_ARG_END} },
  { "p_neqatom",  INSTR_NEQATOM,   {SPEC_ARG_USE_ATOM,SPEC_ARG_USE_ATOM,                                  SPEC_ARG_END} },
  { "freelink",   INSTR_FREELINK,  {SPEC_ARG_USE_LINK,                                                    SPEC_ARG_END} },
  { "freeatom",   INSTR_FREEATOM,  {SPEC_ARG_USE_ATOM,                                                    SPEC_ARG_END} },
  { "unconnect",  INSTR_UNCONNECT, {SPEC_ARG_SET_LINK,SPEC_ARG_USE_LINK,                                  SPEC_ARG_END} },
  { "newlink",    INSTR_NEWLINK,   {SPEC_ARG_SET_LINK,                                                    SPEC_ARG_END} },
  { "bebuddy",    INSTR_BEBUDDY,   {SPEC_ARG_USE_LINK,SPEC_ARG_USE_LINK,                                  SPEC_ARG_END} },
  { "unify",      INSTR_UNIFY,     {SPEC_ARG_USE_LINK,SPEC_ARG_USE_LINK,                                  SPEC_ARG_END} },
  { "call",       INSTR_CALL,      {SPEC_ARG_FUNCTOR, SPEC_ARG_LINKS,                                     SPEC_ARG_END} },
  { "commit",     INSTR_COMMIT,    {                                                                      SPEC_ARG_END} },
  { "success",    INSTR_SUCCESS,   {                                                                      SPEC_ARG_END} },
  { "p_branch",   INSTR_BRANCH,    {SPEC_ARG_BLOCKS,                                                      SPEC_ARG_END} },
  { "newatom",    INSTR_NEWATOM,   {SPEC_ARG_SET_ATOM,SPEC_ARG_FUNCTOR,                                   SPEC_ARG_END} },
  { "setlink",    INSTR_SETLINK,   {SPEC_ARG_USE_LINK,SPEC_ARG_USE_ATOM,SPEC_ARG_INT,                     SPEC_ARG_END} },
  { "failure",    INSTR_FAILURE,   {                                                                      SPEC_ARG_END} },
  { "function",   INSTR_FUNCTION,  {SPEC_ARG_FUNCTOR,                                                     SPEC_ARG_END} },
  { "enqueue",    INSTR_ENQUEUE,   {SPEC_ARG_USE_ATOM,                                                    SPEC_ARG_END} },
  { "recall",     INSTR_RECALL,    {SPEC_ARG_FUNCTOR, SPEC_ARG_LINKS,                                     SPEC_ARG_END} },
  { "p_derefpc",  INSTR_DEREFPC,   {SPEC_ARG_SET_PC,SPEC_ARG_USE_LINK,SPEC_ARG_PCTYPE,                  SPEC_ARG_END} },
  { "copypc",     INSTR_COPYPC,    {SPEC_ARG_USE_LINK, SPEC_ARG_USE_PC,SPEC_ARG_PCTYPE,                 SPEC_ARG_END} },
  { "movepc",     INSTR_MOVEPC,    {SPEC_ARG_USE_LINK, SPEC_ARG_USE_PC,SPEC_ARG_PCTYPE,                 SPEC_ARG_END} },
  { "freepc",     INSTR_FREEPC,    {SPEC_ARG_USE_PC,SPEC_ARG_PCTYPE,                                    SPEC_ARG_END} },
  { "newint",     INSTR_NEWINT,    {SPEC_ARG_SET_INT, SPEC_ARG_INT,                                       SPEC_ARG_END} },
  { "setint",     INSTR_SETINT,    {SPEC_ARG_USE_LINK, SPEC_ARG_USE_INT,                                  SPEC_ARG_END} },
  { "p_derefint", INSTR_DEREFINT,  {SPEC_ARG_SET_INT, SPEC_ARG_USE_LINK,                                  SPEC_ARG_END} },
  { "p_true",     INSTR_TRUE,      {SPEC_ARG_USE_INT,                                                     SPEC_ARG_END} },
  { "p_false",    INSTR_FALSE,     {SPEC_ARG_USE_INT,                                                     SPEC_ARG_END} },
  { "freeint",    INSTR_FREEINT,   {SPEC_ARG_USE_INT,                                                     SPEC_ARG_END} },
  { "intadd",     INSTR_INTADD,    {SPEC_ARG_SET_INT, SPEC_ARG_USE_INT, SPEC_ARG_USE_INT,                 SPEC_ARG_END} },
  { "intsub",     INSTR_INTSUB,    {SPEC_ARG_SET_INT, SPEC_ARG_USE_INT, SPEC_ARG_USE_INT,                 SPEC_ARG_END} },
  { "intmul",     INSTR_INTMUL,    {SPEC_ARG_SET_INT, SPEC_ARG_USE_INT, SPEC_ARG_USE_INT,                 SPEC_ARG_END} },
  { "intdiv",     INSTR_INTDIV,    {SPEC_ARG_SET_INT, SPEC_ARG_USE_INT, SPEC_ARG_USE_INT,                 SPEC_ARG_END} },
  { "intmod",     INSTR_INTMOD,    {SPEC_ARG_SET_INT, SPEC_ARG_USE_INT, SPEC_ARG_USE_INT,                 SPEC_ARG_END} },
  { "inteq",      INSTR_INTEQ,     {SPEC_ARG_SET_INT, SPEC_ARG_USE_INT, SPEC_ARG_USE_INT,                 SPEC_ARG_END} },
  { "intgt",      INSTR_INTGT,     {SPEC_ARG_SET_INT, SPEC_ARG_USE_INT, SPEC_ARG_USE_INT,                 SPEC_ARG_END} },
};

const il_spec* str_to_spec(const std::string& str)
{
  const int il_specs_size = sizeof(il_specs)/sizeof(il_spec);
  
  for(int i=0; i<il_specs_size; ++i){
    if(str == il_specs[i].str){
      return &il_specs[i];
    }
  }

  // そんな命令ないよ
  return 0;
}

