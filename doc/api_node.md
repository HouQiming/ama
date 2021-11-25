# AST API Reference

Our AST (Abstract Syntax Tree) is a tree formed by homogeneously-typed `Node` objects. The AST is language neutral: the same `Node` constructs are used for every language, be it C, Python, or Javascript. We also use one unified parser, with minor customizations, to parse every language.

Not all language constructs are supported, though. Unrecognized language constructs will be parsed into *raw* nodes, which packs node or token sequences verbatim. We always interpret parenthesis so even raw nodes can still form a tree more useful than raw code strings.

## Creating an AST

The AST is usually parsed from strings or files in Javascript with one of the following APIs:

`ParseCode(code, [options])` parses the provided code string into an AST. This function is also available in C++.

`ParseCurrentFile([options])` parses the current file and returns an AST. The `.data` field of the returned root node stores the file path.

`require('depends').LoadFile(name)` loads a file and returns its AST, or `undefined` if the loading failed. The `.data` field of the returned root node stores the file path. Once a file is successfully loaded, the result is cached and later calls will return the same AST even if it were modified. This is useful for maintaining a partially-edited state.

Alternatively, we can also create an in-line AST by wrapping raw code with `@()` in an ama script.

### Parsing Options

The default parser options are listed by the `__global.default_options` assignment in `module/_init.js`:

```Javascript
__global.default_options = {
	//The default_options tries to be generic enough for any language.
	/////////////////////////
	//whether to enable `#foo` comments
	enable_hash_comment: 0,
	symbols: '!== != && ++ -- -> ... .. :: << <= === == => >= >>> >> || <=> ** .* ->* <<<',
	//we treat # as an identifier character to make C stuff like `#define` more idiosyncratic
	identifier_charset: '0-9A-Za-z_$#',
	number_charset: '0-9bouUlLfFn.eE',
	hex_number_charset: '0-9A-Fa-fx.pPuUlLn',
	exponent_charset: '0-9f',
	//could use 'dgimsuy', but a later JS standard could extend it
	regexp_flags_charset: 'A-Za-z',
	//we don't really look up the Unicode tables: enable_unicode_identifiers means "all high bytes are treated as identifiers"
	enable_unicode_identifiers: 1,
	//if enabled, auto finish dangling strings / parenthesis
	finish_incomplete_code: 0,
	///////////
	//flags controlling individual features
	parse_operators: 1,
	parse_pointed_brackets: 1,
	parse_scoped_statements: 1,
	parse_keyword_statements: 1,
	parse_colon_statements: 1,
	parse_cpp11_lambda: 1,
	parse_declarations: 1,
	parse_cpp_declaration_initialization: 1,
	parse_c_conditional: 1,
	parse_labels: 1,
	parse_air_object: 1,
	parse_indent_as_scope: 0,
	parse_indent_as_scope_but_merge_cpp_ctor_lines: 0,
	parse_c_forward_declarations: 1,
	struct_can_be_type_prefix: 1,
	parse_js_regexp: 1,
	enable_c_include: 1,
	enable_js_require: 1,
	///////////
	//binary operators, each \n denotes a change of priority level, it must be followed by a ' '
	//the 'of' operator is a hack to improve JS for-of parsing
	binary_operators: '||\n &&\n |\n ^\n &\n == != === !==\n < <= > >= in of instanceof\n <=>\n << >> >>>\n + -\n * / %\n **\n as\n .* ->*\n',
	prefix_operators: '++ -- ! ~ + - * && & typeof void delete sizeof await co_await new const volatile unsigned signed long short',
	postfix_operators: 'const volatile ++ --',
	cv_qualifiers: 'const volatile',
	//the JS `void` is too common in C/C++ to be treated as an operator by default
	named_operators: 'typeof delete sizeof await co_await new in of instanceof as const volatile',
	//unlike generic named_operators, c_type_prefix_operators only make sense when used before another identifier
	c_type_prefix_operators: 'unsigned signed long short',
	ambiguous_type_suffix: '* ** ^ & &&',
	///////////
	keywords_class: 'class struct union namespace interface impl trait',
	keywords_scoped_statement: 'enum if for while do try switch',
	keywords_extension_clause: 'until else elif except catch finally while',
	keywords_function: 'extern function fn def inline',
	keywords_after_class_name: ': extends implements for where',
	keywords_after_prototype: ': -> => throw const noexcept override',
	keywords_not_a_function: 'switch case #define #if #else #elif return',
	keywords_not_variable_name: 'static const volatile private public protected final noexcept throw override virtual operator',
	//`case` is better treated as a part of a label
	//`template` is parsed by the non-scoped statement parser, but it's created as N_SCOPED_STATEMENT
	keywords_statement: 'return typedef using throw goto #pragma #define #undef #if #ifdef #ifndef #elif #else #endif #line break continue template',
	keywords_operator_escape: 'operator',
	///////////
	//codegen
	tab_width: 4,
	tab_indent: 2, //2 for auto
	auto_space: 1,
	auto_curly_bracket: 0,
}
```

## The Node Class

The `Node` class represents an AST node. It's available in C++ as `ama::Node` and available in Javascript as `Node`.

The Javascript `Node` global name refers to a prototype object and cannot be used directly for construction. Nodes have to be constructed with `node_class`-specific `nFoo` global functions, or created with the parsing functions.

The AST makes extensive use of doubly-linked lists. The links are exposed as mutable fields of `Node` with one-letter names: `.c .s .p .v`. Modifying those fields is not memory-safe, even in Javascript. We recommend mutating trees with methods like `Insert` and `Unlink` instead of modifying the fields directly, unless the task is really creative, 

### Fields

**node_class** is the node type, must be a `N_FOO` constant.

**indent_level** stores delta-indent with respect to parent node. For example, in:

```C++
int main(){
	return 0;
}
```

The `return 0;` node has `.indent_level=4` (default tab width) while all other nodes have `.indent_level=0`. Using this relative indentation representation can keep the formatting sane while moving nodes around.

**tmp_flags** pack temporary flags that must not affect code generation

**flags** are persistent flags affecting code generation.

For N_RAW, it stores `opening_char|closing_char<<8`, for example, `@([])` is `nRaw().setFlags(0x5d5b)`

**data** stores the string content of the node, like the operator string for N_BINOP, the variable name for N_REF.

**comments_before** store comments and spaces before the node.

**comments_after** store comments and spaces after the node.

**c** links to the node's first child.

**s** links to the node's next sibling.

**p** links to the node's parent.

**v** links to the node's previous sibling, or `PackTailPointer(last_sibling)` if `this==this->p->c`


**children** is a Javascript-only property which returns an array with all child nodes.

### Common Methods

Unless otherwise specified, the following methods are supported in both C++ and Javascript:

--------------
- `nd.setData(data)`
- `nd.setFlags(flags)`
- `nd.setCommentsBefore(comments)`
- `nd.setCommentsAfter(comments)`
- `nd.setIndent(indent_level)`

Chainable property setters

--------------
- `nd.AdjustIndentLevel(delta)`

Equivalent to `nd.indent_level+=delta;` plus clamping

--------------
- `nd.FreeASTStorage()`
- `nd.DestroyForSymbol()`

Nodes are garbage-collected by `ama::gc()` in C++ or `Node.gc()` in JS, but you can also free them manually with `nd.FreeASTStorage()` or `nd.DestroyForSymbol()`. Debug build checks for double-frees, but no guarantees.

`nd.FreeASTStorage()` frees the entire tree under `nd`. `DestroyForSymbol` additionally returns the `nd.data`.

--------------
- `nd.CloneEx()`
- `nd.Clone()`

Create a clone of the subtree under `nd`. `CloneEx` is only supported in C++ and additionally returns a mapping between original and cloned nodes.

--------------
- `nd.Unlink()`

Unlink a node from its AST. After unlinking, the .c, .p and .s fields are guaranteed to be NULL.

--------------
- `nd.ReplaceWith(nd_new)`

Replace the `nd` with `nd_new` in the AST containing it. It has no effect if `nd` is a root node. If `nd_new` is NULL, `nd` is unlinked instead.

ReplaceWith has a quirk -- you can't directly replace a node with something that parents it. For example, this is invalid:

```C++
nd.ReplaceWith(nParen(nd)); //will fail!
```

Instead, use GetPlaceHolder():

```C++
Node* nd_tmp=GetPlaceHolder();
nd.ReplaceWith(nd_tmp);
nd_tmp.ReplaceWith(nParen(nd));
```

--------------
- `nd.ReplaceUpto(nd_upto, nd_new)`

Replace all nodes between `nd` and `nd_upto` with `nd_new`. `nd_upto` is inclusive, set `nd_new` to NULL to unlink the nodes instead. `nd_upto` must be a sibling of `nd`. The sibling links between `nd` and `nd_upto` are retained.

--------------
- `nd.Insert(pos, nd_new)`

Insert `nd_new` in a position relative to `nd`. `pos` controls the positioning:

- **POS_BEFORE**: `nd_new` will be the previous sibling of `nd`
- **POS_AFTER**: `nd_new` will be the previous sibling of `nd`
- **POS_FRONT**: `nd_new` will be the previous sibling of `nd`
- **POS_BACK**: `nd_new` will be the previous sibling of `nd`
- **POS_REPLACE**: `nd_new` will be the previous sibling of `nd`

`nd_new` can have sibling chains, i.e., if `nd_new.s != NULL`, the linked siblings will be inserted as well.

--------------
- `nd.Root()`
- `nd.RootStatement()`
- `nd.ParentStatement()`
- `nd.FirstChild()`
- `nd.LastChild()`
- `nd.Prev()`
- `nd.Next()`
- `nd.Parent()`

Find a node with some relationship specified by the method name to `nd`, return NULL if not found.

The definition of statements are lexical: they are immediate children of `N_FILE` or `N_SCOPE`

--------------
- `nd.isRawNode(ch_open, ch_close)`
- `nd.isMethodCall(name)`
- `nd.isSymbol(name)`
- `nd.isRef(name)`

Test for specific nodes. They are cheaper than the Javascript-only `nd.Match`.

--------------
- `nd.isAncestorOf(nd_maybe_child)`

Return true if `nd` is an ancestor of `nd_maybe_child`

--------------
- `nd.Owning(nc)`

Find an ancestor node with node_class nc, return NULL if not found

--------------
- `nd.Owner()`
- `nd.CommonAncestor(b)`

Find the owning `N_CLASS` or `N_FUNCTION`, return `nd.Root()` if neither is found

--------------
- `nd.Find(node_class, data)`
- `nd.FindAll(node_class[, data])`
- `nd.FindAllWithin(boundary, node_class[, data])`
- `nd.FindAllBefore(nd_before, boundary, node_class[, data])`

Find all nodes with a particular class and optionally a specific `.data`.

`boundary` is a bit mask specifying boundaries not to cross:

- **BOUNDARY_FUNCTION** prevents searching into functions
- **BOUNDARY_CLASS** prevents searching into classes
- **BOUNDARY_NODEOF** prevents searching into `N_NODEOF` constructs
- **BOUNDARY_SCOPE** prevents searching into scopes
- **BOUNDARY_MATCH** prevents recursion into children of already-matching nodes
- **BOUNDARY_ONE_LEVEL** limits the search to one level

`nd.Find` returns the first match or NULL if not found. `nd.FindAll...` return an array with all matches. `nd.FindAllBefore` searches in pre-order traversal and stops before visiting `nd_before`.

--------------
- `nd.GetStringValue()`

Returns the string content for `N_STRING`.

The default parser keeps quotes around source code strings, so parsing `"hello world"` gives a node with `{data:"\"hello world\""}` You need `GetStringValue()` to get the content `"hello world"`. Once you call `GetStringValue()` though, the original textual form is no longer preserved. For example, `"hello \u0077orld"` becomes `"hello world"`. That's why this function is not `const`.

--------------
- `nd.GetName()`

Get a best-effort "name" of the node. For example, the `.data` field of `N_CALL` is always empty, but .GetName() on such nodes return the callee's name if it's `N_REF` or `N_DOT`.

--------------
- `nd.dot(name)`

Create N_DOT with this node as the object

--------------
- `nd.toSource()`

Return the full source code of a node. The result can be parsed to `ParseCode` to recreate an equivalent AST.

Implemented in src/codegen/gen.jc

--------------
- `nd.dump()`

Return an abbreviated string dump of the code, useful in error messages.

Implemented in src/codegen/gen.jc

--------------
- `nd.InsertDependency(flags, name)`

Insert a `N_DEPENDENCY` with the specific name and flags if it's not already in the code.

--------------
- `nd.InsertCommentBefore(s)`

Insert more comment before `nd`

--------------
- `nd.MergeCommentsBefore(nd_before)`
- `nd.MergeCommentsAfter(nd_after)`
- `nd.MergeCommentsAndIndentAfter(nd_after)`

Move their comments into `nd` before merging another node.

--------------
- `nd.Validate()`
- `nd.ValidateEx(max_depth, quiet)`

Validates the pointer well-formed-ness of the entire AST under `nd`.

`Validate` prints some message and aborts when there is error. `ValidateEx` returns the errors count instead of aborting if quiet==1. `max_depth` is the maximum allowed AST depth. Anything deeper is considered ill-formed.

--------------
- `nd.ValidateChildCount(n_children, quiet)`

Return true if `nd` has exactly `n_children` children.

--------------
- `nd.BreakSibling()`

Break off all siblings after `nd` as a linked list, starting from `nd.s`, and return its head. Unlike `nd.Unlink()`, the broken-off nodes retain their sibling pointers. Also it's valid to call `nd.BreakSibling()` when `nd.s` is NULL. It just returns NULL.

--------------
- `nd.BreakChild()`

Break off all children of `nd` as a linked list and return its head.

--------------
- `nd.BreakSelf()`

Break off `nd` alongside its siblings as a linked list, and return `nd`.

--------------
- `nd.toSingleNode()`

Convert a node with possible chained siblings into a single node. Usually used in conjunction with `nd.BreakFoo`. The created node can be `N_RAW`, `nd` itself or `N_AIR`.

With -fno-delete-null-pointer-checks, NULL->toSingleNode() returns nAir().

--------------
- `nd.PreorderNext(nd_root)`
- `nd.PreorderSkip()`
- `nd.PostorderFirst()`
- `nd.PostorderNext(nd_root)`

Recursion-free AST traversal. For a recursion-free preorder traversal of the subtree under `nd`:

```C++
for(Node* ndi=nd;ndi;ndi=ndi->PreorderNext(nd)){
	if we want to skip ndi's children {
		ndi=ndi->PreorderSkip();
		continue;
	}
	...
}
```

For a postorder traversal:

```C++
for(Node* ndi=nd->PostorderFirst();ndi;ndi=ndi->PostorderNext(nd)){
	...
}
```

--------------
- `nd.Unparse()`

Unparse turns a node back to a less-parsed state (usually N_RAW). It's mainly used to correct mistakes in an earlier parsing step.

--------------
- `nd.FormatFancyMessage(msg, flags)`

Format a clang-style message, referencing `this` We intentionally limit our messages to warnings and notes

--------------
- `nd.ComputeLineNumber()`

ComputeLineNumber currently traverses the entire AST up to `this`

--------------
- `nd.GetCFGRole()`

Return the role of `nd` in a CFG (Control Flow Graph):

- **CFG_BASIC**: if `nd` is a basic expression / statement
- **CFG_BRANCH**: if `nd` is a branching statement
- **CFG_LOOP**: if `nd` is a loop
- **CFG_JUMP**: if `nd` is a goto statement
- **CFG_DECL**: if `nd` is a declaration that does not map to executable code

--------------
- `nd.isChildCFGDependent(nd_child)`

Return 1 if whether `nd_child` gets executed depends on `nd`, `nd_child` must be a child of `nd`.


--------------
- `nd.NodeofToASTExpression()`

Translate `@()` to Javascript node construction expression. Useful if we eventually run the AST in our own scripting context.

--------------
- `nd.AutoFormat()`

Format the AST with sane indentations. Useful on generated code before dumping.

--------------
- `Node.GetPlaceHolder()` (Javascript)
- `ama::GetPlaceHolder()` (C++)

Get a placeholder node required when adding code around an existing node, see `nd.ReplaceWith` for details.

### Javascript Methods

The following methods are only supported in Javascript:

--------------
- `nd.call(...args)`

Create `N_CALL` with `nd` as the function and `args` as arguments.

--------------
- `nd.enclose(s_brackets)`

Return an enclosure of the current node in `s_brackets`. `s_brackets` can be '[]' or '()'.

--------------
- `nd.then(f, ...args)`

Chainable syntax sugar for `f(nd, ...args)`.

--------------
- `nd.toJSON()`

Callback for `JSON.stringify(nd)`. The resulting JSON is for human inspection only and not useful when parsed back.

--------------
- `nd.Match(nd_pattern)`
- `nd.MatchAll(nd_pattern)`

Match a code template specified by `nd_pattern`. `nd.Match` only checks `nd` itself. `nd.MatchAll` matches the pattern against the entire subtree under `nd` and returns an array of matches.

The returned match objects have the shape `{nd:<matched node>}`

--------------
- `Node.MatchAny([node_class, ]name)`
- `Node.MatchDot(nd_object, name)`

Wildcards for template matching. You can invoke the patterns with nested `@()` inside `@()`.

MatchAny matches any node of an optional node class and saves the result in the `name` property of the returned match.

For example, this code:

```Javascript
let nd_source = @(test(3));
nd_source.Match(
    @(test(@(Node.MatchAny("val"))))
)
```

will return `{nd:nd_source,val:nd_source.Find(N_NUMBER, '3')}`.

--------------
- `nd.Subst(match)`

Substitution a match into the code template specified by `nd`. `@(foo)` under `nd` will be replaced with `match.foo`.

For example, this code:

```Javascript
let nd_source = @(test(@(val)));
nd_source.Subst({val:nNumber(3)})
```

will return `@(test(3))`

--------------
- `nd.Save([options])`

Save the node. `options` can be:

- A string starting with '.' interpreted as a new extension
- A string specifying a full path
- An object with saving options, see [Parsing Options](#-parsing-options). `options.name` is the full path

For example, `nd.Save('.audit.cpp')` saves the current AST into a new file with the same file name as `nd.Root().data` and extension '.audit.cpp'.

--------------
- `nd.StripRedundantPrefixSpace(aggressive)`

Remove redundant spaces from the AST, if `aggressive` is true, also remove newlines.

--------------
- `nd.TranslateTemplates(match_jobs, is_forward)`

Perform template substitution. `match_jobs` is an array with objects in the form `{from:Node, to:Node}`. Each match of the `from` pattern will be replaced with its parameters substituted into the corresponding `to` pattern with `Node.Subst`.

If `is_forward` is false, do it backwards.

--------------
- `nd.GetFunctionNameNode()`
- `nd.SmartConvertIndentToScope(options)`

Return the name of `N_FUNCTION`. Returns an empty string if the function is unnamed.


## Node Classes

Here is a list of all node classes, i.e., possible `node_class` values. The AST stores each node's children in the same order as the constructor parameters listed here.

--------------
- Node class: `N_RAW`
- Constructor: `nRaw(...nodes)`

Reserved for token sequences we cannot exactly parse. It can have any number of children. The textual form is the concatenation of its children, optionally enclosed by two ASCII characters:

- `.flags` packes the enclosing characters: `opening_char|closing_char<<8`. A character value of 0 means it's omitten.


--------------
- Node class: `N_SYMBOL`
- Constructor: `nSymbol(string)`

Reserved for symbol tokens unconnected to surrounding nodes:

- `.data` stores the symbol string.


--------------
- Node class: `N_REF`
- Constructor: `nRef(string)`

An identifier, likely a reference:

- `.data` stores the identifier string.
- `.flags` can take a combination of the following values:
  - `REF_WRITTEN` signifies the identifier is being written, like the `foo` in `foo = bar;`
  - `REF_RW` signifies the identifier is being updated, like the `foo` in `foo += bar;`. Must be used with `REF_WRITTEN`. 
  - `REF_DECLARED` signifies the identifier is being declared, like the `foo` in `int foo;`


--------------
- Node class: `N_NUMBER`
- Constructor: `nNumber(string)`

A number:

- `.data` stores the original textual form of the number.


--------------
- Node class: `N_STRING`
- Constructor: `nString(string)`

A string. C characters are also parsed as strings:

- `.data` stores the original textual form of the string if `.flags==0`, or the string content if `.flags==LITERAL_PARSED`

For example, parsing `'a'` gives `{node_class:N_STRING,flags:0,data:"'a'"}`.   While `nString('a')` gives `{node_class:N_STRING,flags:LITERAL_PARSED,data:"a"}`.


--------------
- Node class: `N_NODEOF`
- Constructor: `nNodeof(nd)`

Our own extension to switch between script code and code templates in scripts. The syntax is `@(foo)`. Example uses:

- `nd.Match(@(JSON.parse<@(Node.MatchAny('foo'))>)))`
- `nd_root.Insert(POS_FRONT, @(#include <stdio.h>))`
- `nd_paramlist.Insert(POS_BACK, @(int @(nRef(name))))`

In normal code, `@()` enters the code template mode: it evaluates to an AST node corresponding to the `@()` contents. Nested `@()` switches back to normal code, where one can put in non-verbatim content like `nRef(name)`. Nested `@()` supports sibling chains, like `nd.Insert`.


--------------
- Node class: `N_SCOPE`
- Constructor: `nScope(...nodes)`

A statement block:

- `.flags` can be:
  - `0` to indicate a `{}`-enclosed block.
  - `SCOPE_FROM_INDENT` to indicate a Python-like indentation-based block.


--------------
- Node class: `N_FUNCTION`
- Constructor: `nFunction(nd_before, nd_parameter_list, nd_after, nd_body)`

A function declaration:

- `nd_before` packs everything before the parameter list, e.g. `int __cdecl printf` in `int __cdecl printf(char* fmt,...);`
- `nd_parameter_list` is a `N_PARAMETER_LIST` (documented later) storing the parameter list.
- `nd_after` packs everything between the parameter list and the body, e.g. `const override` in `virtual void foo()const override{}`
- `nd_body` is the function body. It is usually `N_AIR` or `N_SCOPE`, 

but can also be an arbitrary expression for lambda expressions like the `x+1` in `x => x+1`.

- `data` stores the deduced function name


--------------
- Node class: `N_CLASS`
- Constructor: `nClass(keyword, nd_before, nd_name, nd_after, nd_body)`

A class declaration. Take the following code as example:

```C++
struct __attribute__((aligned(16))) Quaternion: Vector4 {
    ...
};
```

- `.data` stores the keyword declaring the class, like `struct` in the example.

It's provided to the constructor with the `keyword` argument.

- `nd_before` packs everything between the keyword and the class name, like `__attribute__((aligned(16)))` in the example.
- `nd_name` is a `N_REF` with the class name, like `Quaternion` in the example.
- `nd_after` packs everything between the class name and the body, like `: Vector4` in the example.
- `nd_body` is the class body. It is usually `N_AIR` or `N_SCOPE`, like `{...}` in the example.


--------------
- Node class: `N_POSTFIX`
- Constructor: `nPostfix(nd, operator)`

A postfix operation:

- `.data`, provided by `operator`, is the operator string.
- `nd` is the operand.


--------------
- Node class: `N_DOT`
- Constructor: `nd.dot(name)`

A dot notation:

- `.data`, provided by `name`, is the member name.
- `nd` is the object.


--------------
- Node class: `N_ITEM`
- Constructor: `nItem(nd_object, ...subscripts)`

Array item fetching expression in the form of `nd_object[...subscripts]`. `subscripts` can be empty and `nd_object` can be `N_AIR`. If `subscripts` contain multiple nodes, they are assumed to be comma-separated.


--------------
- Node class: `N_CALL`
- Constructor: `nCall(nd_function, ...parameters)`

Function-call expression in the form of `nd_function(...parameters)`.


--------------
- Node class: `N_CALL_TEMPLATE`
- Constructor: `nCallTemplate(nd_template, ...parameters)`

Template instantiation expression in the form of `nd_template<...parameters>`.


--------------
- Node class: `N_CALL_CUDA_KERNEL`
- Constructor: `nCallCudaKernel(nd_function, ...parameters)`

The parameter binding portion of a CUDA kernel call in the form of `nd_function<<<...parameters>>>`.


--------------
- Node class: `N_DEPENDENCY`
- Constructor: `nDependency(...nodes).setFlags(flags)`

A node with dependency information, like `#include` in and `require` in Javascript_

- `.flags` can be:
  - `DEP_C_INCLUDE` stands for C `#include`, if we also have `.flags&DEPF_C_INCLUDE_NONSTR != 0`, it's `#include <>`.

In any case, `nodes` packs one string. However, it can also be an arbitrary expression in presence of advanced   C preprocessor techniques like `#include ADD_FANCY_DIRECTORY("name." HEADER_EXTENSION)`.

  - `DEP_JS_REQUIRE` stands for Javascript `require`, `nodes` packs one require argument


--------------
- Node class: `N_BINOP`
- Constructor: `nBinop(nd_a, operator, nd_b)`

A binary operation:

- `.data`, provided by `operator`, is the operator string.
- `nd_a` and `nd_b` are the two operands.


--------------
- Node class: `N_PREFIX`
- Constructor: `nPrefix(operator, nd)`

A prefix operation:

- `.data`, provided by `operator`, is the operator string.
- `nd` is the operand.


--------------
- Node class: `N_ASSIGNMENT`
- Constructor: `nAssignment(nd_target, [symbol, ]nd_value)`

An assignment in the form of `nd_target symbol= nd_value`:

- `.data`, provided by `symbol`, packs the assignment symbol sans `=`, like the `+` in `a += 1`.

Use an empty string for plain assignment.

- `nd_target` is the target.
- `nd_value` is the value.


--------------
- Node class: `N_SCOPED_STATEMENT`
- Constructor: `nScopedStatement(keyword, nd_parameter, nd_body[, ...extension_clauses])`

A special statement that canonically takes a statement block, like `if`:

- `.data`, provided by `keyword`, stores the keyword, like `for` in `for(;;){}`.
- `nd_parameter` packs everything between the keyword and the body, like the `(;;)` in `for(;;){}`.

It can be `N_AIR` for statements like `try{}` or `do{}while()`.

- `nd_body` is the statement block. It can take values other than `N_SCOPE` in C when the scope is omitten.
- `extension_clauses` pack follow-ups after the body, like `else` clauses for `if` and `catch` clauses for `try.


--------------
- Node class: `N_EXTENSION_CLAUSE`
- Constructor: `nExtensionClause(keyword, nd_parameter, nd_body)`

An extension clause. Parameters and fields are similar to `N_SCOPED_STATEMENT`.


--------------
- Node class: `N_PARAMETER_LIST`
- Constructor: `nParameterList(...nodes)`

A list of parameters:

- `.flags` can be:
  - `0` stands for a `()`-enclosed list
  - `PARAMLIST_TEMPLATE` stands for a `<>`-enclosed list
  - `PARAMLIST_UNWRAPPED` stands for a free-standing list
- `nodes` are a list of `N_ASSIGNMENT` nodes, the target of each represents a parameter while the value of each represents an optional default value.

Parameters without a default value have it set to `N_AIR`.


--------------
- Node class: `N_CONDITIONAL`
- Constructor: `nConditional(nd_condition, nd_true_value, nd_false_value)`

C `?:` construct in the form of `nd_condition ? nd_true_value : nd_false_value`.


--------------
- Node class: `N_LABELED`
- Constructor: `nLabeled(nd_label, nd_value)`

Generic labeled construct in the form of `nd_label: nd_value`. It can be a number of things, including but not limited to:

- A C goto label with a statement like `end: return;`
- A C case or default clause like `case 0: break;`
- A Javascript field initializer like the inside of `{foo:"bar"}`


--------------
- Node class: `N_AIR`
- Constructor: `nAir()`

An *air node* that stands for something grammatical but generates no code, like the namespace in `::foo`.


--------------
- Node class: `N_FILE`
- Constructor: `nFile(...nodes)`

A source file. Must be the root node:

- `.data` stores the file name when available.
- `.flags` can be:
  - `0` stands for a tab-indented file
  - `FILE_SPACE_INDENT` stands for a space-indented file


--------------
- Node class: `N_SEMICOLON`
- Constructor: `nSemicolon(nd)`

A trailing semicolon: `nd;`


--------------
- Node class: `N_PAREN`
- Constructor: `nParen(nd)`

A pair of parenthesis: `(nd)`


--------------
- Node class: `N_KEYWORD_STATEMENT`
- Constructor: `nKeywordStatement(keyword, nd_parameter)`

A special statement that cannot take a statement block, like `return`:

- `.data`, provided by `keyword`, stores the keyword, like the `goto` in `goto end;`
- `nd_parameter` pack whatever that follows the keyword, or `N_AIR` if there is no follow-up, like in `break;`.


--------------
- Node class: `N_JS_REGEXP`
- Constructor: `nJsRegexp(string)`

A Javascript regular expression in the form of `/foo/flags`.

- `.data` stores the original textual form of the regular expression.



## Notes

`doc/api_node.md` is automatically generated by `script/docgen.ama.js`. If we want to edit this document, edit `script/doc_templates/node.md` instead.
