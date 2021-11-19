# Document for Contributors

## Efforts Needed

- More creative ways of language customization
- Better documentation
- Better support of not-so-C-like languages

## C++11 Dialect for Development

The default development dialect is defined by `modules/cpp/amacpp.js`. You're welcome to customize it in anyway you want, but the base language should work in C++11. Polyfill any library feature needed from later C++ revisions. 

Main features of the default development dialect:
- Indentation-based scoping and statement delimiting. We can write:
```C++
if foo:
	bar()
	baz()
```
And after `script/sync.js` it will become:
```C++
if (foo) {
	bar();
	baz();
}
```
- Shortcut for common types: `i32[]` becomes `std::vector<int32_t>`
- Undeclared variables will be automatically declared as `auto`
- Uninitialized variables will be zero-initialized with `Type foo{};`
- `auto` will be replaced with a deduced type whenever possible
- Global functions default to `static`, prefix with `public` to export

## Big Rules

**No guarantees** Amalang is a best-effort tool and we do not pursue guarantees of any kind. We try our best to be helpful, but we don't make promises we can't keep. 

**Idiosyncrasy** When generating code, try to make the result idiosyncratic. If a generator eventually loses maintenance, another soul could end up hand-tweaking the once-generated code. Also enclose the generated code in some tags so that a backward transformation pass can remove them. Checkout `src/script/jsgen.cpp` for an example.

**Tolerance** Make our AST manipulation coexist with other coding styles and language features, even if we can't parse them or don't like them. Amalang is designed to assist programmers, not patronize.

## Small Rules

Yes, we can break these if we really want to.

- Avoid recursion whenever possible. Our tolerance means we need to deal with insanely deep or wide ASTs. Recursion tends to crash or timeout. If recursion is a must, use `nd.ValidateEx(max_depth)` to limit the depth first.
- Keep each variable declaration on its own line, make all members public. We use Amalang to customize our own development so keeping the syntax simple will make our own scripts easier to write.
- Use `.cpp` and `.hpp` extensions for our own code and other extensions for third-party code. Our scripts need something to differentiate them.
- Everything under `ama::Node` must be garbage-collectible. That means no STL and no memory allocation. It's OK and encouraged to use STL elsewhere.
- It is fine to generate code as text and `ParseCode` the result. We do not discriminate ASTs by means of creation.
