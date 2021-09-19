This file contains information about major changes and features.

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