_Regular expressions_ (or short _regexes_) are often used to check if a text matches a certain pattern. For example the regex `ab?c` would match `abc` or `ac`, but not `abbc` or `123`. In Chatterino, you can use them to highlight messages (and more) based on complex conditions.

Basic patterns:

|Pattern    |Matches|
|-|-|
|`x?`       |nothing or `x`|
|`x*`       |`x`, repeated any number of times|
|`x+`       |`x`, repeated any number of times but at least 1|
|`^`        |The start of the text|
|`$`        |The end of the text|
|`x\|y`      |`x` or `y`|

You can group multiple statements with `()`:

|Pattern    |Matches|
|-|-|
|`asd?`     |`asd` or `as`|
|`(asd)?`   |`asd` or nothing|
|`\(asd\)`  |`(asd)`, literally|

You can also group multiple characters with `[]`:

|Pattern    |Matches|
|-|-|
|`[xyz]`    |`x`, `y` or `z`|
|`[1-5a-f]` |`1`,`2`,`3`,`4`,`5`,`a`,`b`,`c`,`d`,`e`,`f`|
|`[^abc]`   |Anything, **except** `a`, `b` and `c`|
|`[\-]`     |`-`, literally (escaped with `\`)|
|`\[xyz\]`  |`[xyz]`, literally|

Special patterns:

|Pattern    |Matches|
|-|-|
|`\d`       |Digit characters (0-9)|
|`\D`       |Non-digit characters|
|`\w`       |Word characters (a-zA-Z0-9_)|
|`\W`       |Non-word characters|
|`\s`       |Spaces, tabs, etc.|
|`\S`       |Not spaces, tabs, etc.|
|`\b`       |Word boundaries (between \w and \W)|
|`\B`       |Non-word boundaries|

You can try out your regex pattern on websites like [https://regex101.com/](regex101.com).