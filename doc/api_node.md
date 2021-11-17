# AST API Reference

TODO: what is AST

## Creating an AST

The AST (Abstract Syntax Tree) is usually created in Javascript with a parsing API:

`ParseCode(code, [options])` parses the provided code string into an AST. This function is also available in C++.

`ParseCurrentFile([options])` parses the current file and returns an AST. The `.data` field of the returned root node stores the file path.

`require('depends').LoadFile(name)` loads a file and returns its AST, or `undefined` if the loading failed. The `.data` field of the returned root node stores the file path. Once a file is successfully loaded, the result is cached and later calls will return the same AST even if it were modified. This is useful for maintaining a partially-edited state.

Alternatively, we can also create an in-line AST by wrapping raw code with `.()` in an ama script.

### Parsing Options

TODO: default_options

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

The `return 0;` node has `.indent_level=4` (default tab width) while all other nodes have `.indent_level=0`.

**tmp_flags** pack temporary flags that must not affect code generation

**flags** are persistent flags affecting code generation. For N_RAW, it stores `opening_char|closing_char<<8`, for example, `.([])` is `nRaw().setFlags(0x5d5b)`

**data** stores the string content of the node, like the operator string for N_BINOP, the variable name for N_REF.

**comments_before** store comments and spaces before the node.

**comments_after** store comments and spaces after the node.

**c** links to the node's first child.

**s** links to the node's next sibling.

**p** links to the node's parent.

**v** links to the node's previous sibling, or `PackTailPointer(last_sibling)` if `this==this->p->c`


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

Nodes are garbage-collected by `ama::gc()` in C++ or `Node.gc()` in JS, but you can also free them manually with `nd.FreeASTStorage()` or `nd.DestroyForSymbol()`. Debug build checks for double-frees, but no guarantees. `nd.FreeASTStorage()` frees the entire tree under `nd`. `DestroyForSymbol` additionally returns the `nd.data`.

--------------
- `nd.CloneEx()`
- `nd.Clone()`

Create a clone of the subtree under `nd`. `CloneEx` is only supported in C++ and additionally returns a mapping between original and cloned nodes.

--------------
- `nd.Unlink()`

Unlink a node from its AST. After unlinking, the .c, .p and .s fields are guaranteed to be NULL.

--------------
- `nd.ReplaceWith(nd_new)`

Replace the `nd` with `nd_new` in the AST containing it. It has no effect if `nd` is a root node. If `nd_new` is NULL, `nd` is unlinked instead. ReplaceWith has a quirk -- you can't directly replace a node with something that parents it. For example, this is invalid:

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

--------------
- `nd.Root()`
- `nd.RootStatement()`
- `nd.ParentStatement()`
- `nd.LastChild()`
- `nd.Prev()`

Find a node with some relationship specified by the method name to `nd`, return NULL if not found. The definition of statements are lexical: they are immediate children of `N_FILE` or `N_SCOPE`

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

Find all nodes with a particular class and optionally a specific `.data`. `boundary` is a bit mask specifying boundaries not to cross:

- **BOUNDARY_FUNCTION** prevents searching into functions
- **BOUNDARY_CLASS** prevents searching into classes
- **BOUNDARY_NODEOF** prevents searching into `N_NODEOF` constructs
- **BOUNDARY_SCOPE** prevents searching into scopes
- **BOUNDARY_MATCH** prevents recursion into children of already-matching nodes
- **BOUNDARY_ONE_LEVEL** limits the search to one level

`nd.Find` returns the first match or NULL if not found. `nd.FindAll...` return an array with all matches. `nd.FindAllBefore` searches in pre-order traversal and stops before visiting `nd_before`.

--------------
- `nd.GetStringValue()`

Returns the string content for N_STRING. The default parser keeps quotes around source code strings, so parsing "hello world" gives a node with .data="\"hello world\"" You need GetStringValue() to get the content "hello world". Once you call GetStringValue() though, the original textual form is no longer preserved. For example, "hello \u0077orld" becomes "hello world". That's why this function is not `const`.

--------------
- `nd.GetName()`

Get a best-effort "name" of the node. For example, the `.data` field of `N_CALL` is always empty, but .GetName() on such nodes return the callee's name if it's `N_REF` or `N_DOT`.

--------------
- `nd.dot(name)`

Create N_DOT with this node as the object

--------------
- `nd.toSource()`

Return the full source code of a node. The result can be parsed to `ParseCode` to recreate an equivalent AST. Implemented in src/codegen/gen.jc

--------------
- `nd.dump()`

Return an abbreviated string dump of the code, useful in error messages. Implemented in src/codegen/gen.jc

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

Validates the pointer well-formed-ness of the entire AST under `nd`. `Validate` prints some message and aborts when there is error. `ValidateEx` returns the errors count instead of aborting if quiet==1. `max_depth` is the maximum allowed AST depth. Anything deeper is considered ill-formed.

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

Convert a node with possible chained siblings into a single node. Usually used in conjunction with `nd.BreakFoo`. The created node can be `N_RAW`, `nd` itself or `N_AIR`. With -fno-delete-null-pointer-checks, NULL->toSingleNode() returns nAir().

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

Translate `.()` to Javascript node construction expression. Useful if we eventually run the AST in our own scripting context.

--------------
- `nd.AutoFormat()`

Format the AST with sane indentations. Useful on generated code before dumping.

### Methods

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

Match a code template specified by `nd_pattern`. `nd.Match` only checks `nd` itself. `nd.MatchAll` matches the pattern against the entire subtree under `nd` and returns an array of matches. The returned match objects have the shape `{nd:<matched node>}`

--------------
- `nd.MatchAny([node_class, ]name)`
- `nd.MatchDot(nd_object, name)`

Wildcards for template matching. You can invoke the patterns with nested `.()` inside `.()`. MatchAny matches any node of an optional node class and saves the result in the `name` property of the returned match. For example, this code:

```Javascript
let nd_source = .(test(3));
nd_source.Match(
    .(test(.(Node.MatchAny("val"))))
)
```

will return `{nd:nd_source,val:nd_source.Find(N_NUMBER, '3')}`.

--------------
- `nd.Subst(match)`

Substitution a match into the code template specified by `nd`. `.(foo)` under `nd` will be replaced with `match.foo`. For example, this code:

```Javascript
let nd_source = .(test(.(val)));
nd_source.Subst({val:nNumber(3)})
```

will return `.(test(3))`

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

Perform template substitution. `match_jobs` is an array with objects in the form `{from:Node, to:Node}`. Each match of the `from` pattern will be replaced with its parameters substituted into the corresponding `to` pattern with `Node.Subst`. If `is_forward` is false, do it backwards.

--------------
- `nd.GetFunctionNameNode()`

Return the name of `N_FUNCTION`. Returns an empty string if the function is unnamed.

--------------
- `nd.GetCompleteParseOption(options)`

Create an extension-aware parsing option for `ParseCode`.


## Node Types


## Notes

`doc/api_node.md` is automatically generated by `script/docgen.ama.js`. If we want to edit this document, edit `script/doc_templates/node.md` instead.
