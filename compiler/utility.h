#ifndef UTILITY_H_INCLUDED
#define UTILITY_H_INCLUDED

#include <vector>
#include <sstream>
#include <ostream>

struct deleter
{
  template<typename T> void operator()(T* x) { delete x; }
};

template<typename T> void delete_vector(std::vector<T*>* v)
{
  for(unsigned int i=0; i<v->size(); ++i) delete (*v)[i];
  delete v;
}

template<typename T> std::string to_string(const T& x)
{
  std::stringstream ss;
  ss << x;
  return ss.str();
}

class ntimes_space
{
public:
  int data;
  ntimes_space(int d) : data(d) {}
};

std::ostream& operator <<(std::ostream& o, const ntimes_space& x);

struct location
{
  int line;
  int column;
};

std::ostream& operator <<(std::ostream& o, const location& x);

#endif
