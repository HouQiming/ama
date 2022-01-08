-- ama fac.hs && ghc fac.audit.hs && ./fac.audit
-- @ama
-- __pipeline.unshift(require('cpp/cpp_indent').setup);
-- __pipeline.push(
--   require('cpp/cpp_indent'),
--   require('./c2hs.ama.js'),
--   {change_ext:'.audit.hs'},
--   'Save'
-- )

int fac(int a)
  if a == 0
    return 1
  else
    return a * fac(a-1)
 
int main()
  for i<10
    console.log(fac(i));
  return 0
