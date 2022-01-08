# Haskell Example

Shows how to translate code into a different paradigm.

## Fizzbuzz

`fizzbuzz.hs` demonstrates a translation from C to Haskell Monads.

To run:

```sh
ama -f cpp/cpp_indent \
	-f ./c2hs.ama.js \
	-f '{"change_ext":".audit.hs"}' \
	-f Save fizzbuzz.hs \
ghc fizzbuzz.audit.hs
./fizzbuzz.audit
```

## Factorial

`fac.hs` demonstrates a translation from a pure C function to a pure Haskell function.

To run:

```sh
ama -f cpp/cpp_indent \
	-f ./c2hs.ama.js \
	-f '{"change_ext":".audit.hs"}' \
	-f Save fac.hs \
ghc fac.audit.hs
./fac.audit
```

## Credit

Inspired by [a reddit comment](https://www.reddit.com/r/ProgrammingLanguages/comments/rxyldk/comment/hrpgix2/?utm_source=share&utm_medium=web2x&context=3).
