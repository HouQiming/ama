# Filter and Module Reference

## Filters

Here is a list of filters intended for the `-f` command line option or the `filters` option of `bisync`. Each filter can be:

- A string indicating a built-in filter, like `"Save"`
- A `function(nd_root, options)` that operates on the AST `nd_root`. In the command line, a function in the form of `require("foo/bar").baz` are abbreviated as `foo/bar.baz`
- An object with option changes

### Save

`"Save"` Save the file, including changes from previous filters.

Put an option object `{change_ext:'.foo'}` before `"Save"` to save the changes under a different extension.

### StripRedundantPrefixSpace

`"StripRedundantPrefixSpace"` Strip redundant spaces.

【filters】
## Modules

Amalang uses the CommonJS module system. To load a module, use `require('module_path')`.

Here is a list of modules for ama script development.

【modules】
