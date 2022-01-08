-- ama -f cpp/cpp_indent -f ./c2hs.ama.js -f '{"change_ext":".audit.hs"}' -f Save fizzbuzz.hs && ghc fizzbuzz.audit.hs && ./fizzbuzz.audit
 
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
