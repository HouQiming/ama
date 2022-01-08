-- ama -f cpp/cpp_indent -f ./c2hs.ama.js -f '{"change_ext":".audit.hs"}' -f Save fac.hs && ghc fac.audit.hs && ./fac.audit

int fac(int a)
  if a == 0
    return 1
  else
    return a * fac(a-1)
 
int main()
  for i<10
    console.log(fac(i));
  return 0
