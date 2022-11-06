This file contains information about major changes and features.

# Upcoming (0.6)

* Supertalk source data is now stored inside assets (editor-only)
* Supertalk assets now show the original file content.
  * Assets from previous versions must be reimported for this to work (this only affects the ability to view source data, old script assets will still work fine)
* An experimental editor for Supertalk assets has been implemented.
  * This must be enabled in Project Settings > Editor > Supertalk Editor. Restart the editor after changing this setting.
  * This also lets you create Supertalk scripts within the editor without having to import from an external file.
  * The editor is somewhat untested and due to how it works it is possible to lose script data. Be careful when enabling this option!
  * Old script assets must be reimported before they can be edited from within the editor.
* Script assets can now be exported to text files if they were imported after this version.
* Script assets are now compiled upon saving (incl. when packaging).
  * Old assets that don't have source data available are an exception to this.
  * This means that referencing a script that fails to compile will fail packaging.

# 0.5

* Property-based variable providers
  * Supports passing any UObject to the Supertalk player and accessing properties on it from scripts.
* `USupertalkObjectValue` now lets you access exposed properties.
  * Initial `TMap` support (only for maps with `FString`/`FName`/`FText` keys)
* Added an editor-only "notes" field to `FSupertalkTableRow`

# 0.4

* Function calls can now have variables passed to them as arguments.
  * This is somewhat hacky at the moment, still requiring arguments to be parsed out from a string. This functionality will eventually be rewritten.
* Function-based variable providers
  * When a variable can't be found by the supertalk player, it runs these functions to allow them to provide it instead.

# 0.3

* Choices can now have localization keys applied.
  * This has been implemented in a somewhat hacky way to get around limitations of the lexer.
* Default localization namespace changed to `Supertalk.Script.Default` from `Script.Default`, to match documentation.

# 0.2

* Parallel actions now use `[` and `]`
* Queued actions now use `{` and `}` (this used to be used for parallels)
* `(` and `)` are now for grouping expressions.
* "Names" (unquoted strings/bare words) can no longer contain whitespace. If you need whitespace, use a quoted
  string.
  * Jumps and sections are unaffected - these can still contain spaces. 
* Basic expressions are now supported (`==`, `~` for not as `!` is already taken, `~=`)
  * Further logical operators will be added in the future. Math operators will come later.
  * Supported in if statements and assignment.
  * Not supported in formatting markup inside text.
    * This is unlikely to supported any time soon (if ever) as it would require either running the expression parser at
      runtime or pre-parsing text formatting and then modifying it.
* Long-form `if ... then ... else ...` actions have been added.
  * Shorthand `value? true, false` conditionals have been removed entirely due to complications with expression parsing.