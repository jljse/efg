#include <iostream>
#include "parser.h"
#include "compiler.h"
#include "optimizer.h"
#include "generator.h"

int main(int argc, char *argv[])
{
  char mode = '0';

  if(argc == 1){
    std::cerr << "usage: efg-bin.exe [-pnco]" << std::endl;
    std::cerr << "-p : parsing" << std::endl;
    std::cerr << "-n : compile node" << std::endl;
    std::cerr << "-c : il code" << std::endl;
    std::cerr << "-o : optimized il code" << std::endl;
    std::cerr << "   : c code" << std::endl;
    return 1;
  }
  
  for(int i=1; i<argc; ++i) {
    if(argv[i][0] == '-'){
      mode = argv[i][1];
      continue;
    }
    
    parser p;
    p_node::p_root* pr = p.parse(argv[i]);
    if(! pr) exit(1);
    if(mode == 'p'){ pr->output(std::cout, 0); return 0; }
    
    compiler c;
    c_node::c_root* cr = c.to_c_node(pr);
    delete pr;
    if(! cr) exit(1);
    if(mode == 'n'){ cr->output(std::cout); return 0; }

    il_node::il_root* ir = c.compile(cr);
    delete cr;
    if(! ir) exit(1);
    if(mode == 'c'){ ir->output(std::cout); return 0; }

    optimizer o;
    o.optimize(ir);
    if(mode == 'o'){ ir->output(std::cout); return 0; }

    generator g;
    g.generate(ir, std::cout);
    
    delete ir;
  }
  
  return 0;
}
