#include "utility.h"

std::ostream& operator <<(std::ostream& o, const ntimes_space& x)
{
  for(int i=0; i<x.data; ++i){
    o << "  ";
  }
  return o;
}

std::ostream& operator <<(std::ostream& o, const location& x)
{
  o << "(" << x.line << "," << x.column << ")";
  return o;
}
