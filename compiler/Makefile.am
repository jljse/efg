BUILT_SOURCES = p_lex.cc p_yacc.hh p_yacc.cc stack.hh position.hh location.hh p_yacc.output
bin_PROGRAMS = efg-bin

noinst_HEADERS = \
c_node.h  compiler.h  c_to_il.h  generator.h  il_node.h  il_spec.h  optimizer.h  parser.h  p_node.h  p_to_c.h  utility.h \
p_yacc.hh stack.hh position.hh location.hh

efg_bin_SOURCES = \
c_node.cpp    c_to_il.cpp    il_node.cpp  main.cpp       parser.cpp  p_to_c.cpp \
compiler.cpp  generator.cpp  il_spec.cpp  optimizer.cpp  p_node.cpp  utility.cpp \
p_lex.l_ p_yacc.y_ \
p_lex.cc p_yacc.cc

stack.hh position.hh location.hh p_yacc.hh p_yacc.cc: p_yacc.y_
	bison -d -ra -op_yacc.cc $<

p_lex.cc: p_lex.l_
	flex -8 -o$@ $<
