-- ama fizzbuzz.hs && ghc fizzbuzz.audit.hs && ./fizzbuzz.audit
-- @ama
-- __pipeline.unshift(require('cpp/cpp_indent').setup);
-- __pipeline.push(
--   require('cpp/cpp_indent'),
--   require('./c2hs.ama.js'),
--   {change_ext:'.audit.hs'},
--   'Save'
-- )
 
int main()
  for i<10
    if i%3==0&&i%5==0
      console.log("FizzBuzz");
    else if i%3==0
      console.log("Fizz");
    else if i%5==0
      console.log("Buzz");
    else
      console.log(i);
  return 0
