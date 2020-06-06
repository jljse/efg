#ifndef C_TO_IL_H_INCLUDED
#define C_TO_IL_H_INCLUDED

#include <map>
#include <vector>
#include <stack>
#include <list>
#include <set>
#include "il_node.h"

// �۴�include����
namespace c_node
{
  class c_atomic;
  class c_link;
};

// �롼��Υޥå�����������
class matching_context
{
public:
  // �ޥå�/�����ѤߤΥ��ȥ�(map:���ȥ४�֥�������->����index)
  typedef std::map<const c_node::c_atomic*,int> found_atom_type;
  found_atom_type found_atom;
  // �ޥå�/�����ѤߤΥ��(map:��󥯥��֥�������->����index)
  typedef std::map<const c_node::c_link*,int> found_link_type;
  found_link_type found_link;
  // isbuddy�ǥ롼�פ�����å��ѤߤΥ��
  std::set<const c_node::c_link*> isbuddy_checked;
  
  int next_index;

  matching_context() { next_index = 0; } // 0�֤ϼ�Ƴ���ȥ�
  // �ƥޥå����оݤ��ޥå�/�������Ƥ���Ф���Τ���index���֤�, �ޥå�/�������Ƥ��ʤ����-1���֤�
  int get_index(const c_node::c_atomic* x) const;
  int get_index(const c_node::c_link* x) const;

  // ���оݤ�index��դ� ��ФǤ��뤳�Ȥ���� index���֤�
  int add_index(const c_node::c_atomic* x);
  int add_index(const c_node::c_link* x);

  // �롼�ץ����å�����λ���Ƥ��뤫�ɤ���
  bool is_isbuddy_checked(const c_node::c_link* x);
  // �롼�ץ����å��Ѥߤ�Ͽ
  void add_isbuddy_checked(const c_node::c_link* x);
};

// ���Ϥ���il����������
class output_il_context
{
public:
  // ���Ϥ����롼��
  std::vector<std::list<il_node::il_operator*>*> result;
  // ������֥�å�
  std::stack<std::list<il_node::il_operator*>*> block_stack;
  
  // �롼����Ϥ򳫻Ϥ���
  void begin_rule();
  // ������Υ롼���λ����
  void end_rule();
  // ��������Υ֥�å���̿������
  void output(il_node::il_operator*);
  // ��������Υ֥�å���̿�����Ϥ�����,������Υ֥�å����ѹ�����
  void output_begin_block(il_node::il_operator*, std::list<il_node::il_operator*>*);
  // ��������Υ֥�å����Ĥ�,��������Υ֥�å��˽�������ѹ�����
  void end_block();
  // ���Ϸ�̤����� (������Ϳ����vector��swap�Ƿ�̤��֤�)
  void swap_result(std::vector<std::list<il_node::il_operator*>*>&);
};

#include "c_node.h"

#endif
