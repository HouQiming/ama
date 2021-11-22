# Javascript Module Reference

Amalang uses the CommonJS module system. To load a module, use `require('module_path')`.

## Modules

Here is a list of modules for ama script development.

【modules】
## Filters

Here is a list of filters intended for the `filters` option of `bisync`. Each filter can be:

- A string indicating a built-in filter, like `"Save"`
- A `function(nd_root, options)` that operates on the AST `nd_root`
- An object with option changes

### Save

`"Save"` Save the file, including changes from previous filters.

Put an option object `{change_ext:'.foo'}` before `"Save"` to save the changes under a different extension.

### StripRedundantPrefixSpace

`"StripRedundantPrefixSpace"` Strip redundant spaces.

【filters】
