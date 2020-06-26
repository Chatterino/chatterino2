## Introduction to Filters

Filters can be applied to splits to provide a selective view of messages. Filters are created in the Settings page and are applied by opening the Split menu (three dots) and selecting "Set filters". Applied filters are saved when you close and open Chatterino.

Multiple filters can be applied to a Split. A message must pass all applied filters for it to be displayed.

Filter definitions take inspiration from many programming languages, and the full description of keywords and operators can be found below.

## Example filters
- `message.content contains "hello"`
    - Only messages that contain the phrase `hello`
- `message.length < 40 || author.subscribed`
    - Messages that are less than 40 characters log, OR are sent by a subscriber.
- `channel.name == "somestreamer" && author.badges contains "moderator"`
    - Messages that originated in the channel `somestreamer` AND are from users with a moderator badge

## Filter syntax

A filter must be a valid expression. An expression is comprised of conditions and values which are evaluated to a single `True` or `False` value to decide whether to filter a message.
Evaluating to something other than `True` or `False` will lead to all messages being filtered out.

### Values
A value can be an integer (`123`, `5`), a string (`"hello"`, `"this is a string"`), or a variable (`author.name`, `message.length`).

When a filter is evaluated, variables are replaced with the values they represent.


**Literals:**
| Name | Example |
| - | - |
| Int | `123`, `5` |
| String | `"Hello there"`, `"Escaped \" quote"` |

### Operators

Binary operators act on two values:
- `1 + 2`
- `author.subbed && flags.highlighted`
- `"long sentence" contains "ten"`

Unary operators act on one value:
- `!author.subbed`

The following operators are available:
| Operator | Description |
| - | - |
| `&&` | And |
| `\|\|` | Or |
| `!` | Not |
| `==` | Equals |
| `!=` | Not equals |
| `<` | Less than |
| `<=` | Less than or equal to |
| `>` | Greater than |
| `>=` | Greater than or equal to |
| `contains` | String, List, or Map contains |
| `+` | Add (or string concatenation) |
| `-` | Subtract |
| `*` | Multiply |
| `/` | Divide (integer) |
| `%` | Modulus |

### Variables

The following variables are available:

| Variable | Type | Description |
| - | - | - |
| **Author** | | User who sent the message|
| `author.badges` | List<String> | List of author's badges |
| `author.color` | Color* | Color code of author, or none |
| `author.name` | String | Display name of author |
| `author.no_color` | Bool | Whether the author has no color set (i.e. gray name) |
| `author.subbed` | Bool | Whether author is subscribed |
| `author.sub_length` | Int | How long author has been subscribed (or zero) |
| **Channel** | | The channel where the message was sent |
| `channel.name` | String | Channel name |
| `channel.watching` | Bool | Whether the channel is being watched (requires Chatterino extension) |
| **Flags** | | Message-specific flags |
| `flags.highlighted` | Bool | Whether the message is highlighted |
| `flags.points_redeemed` | Bool | Whether the message was redeemed through channel points |
| `flags.sub_message` | Bool | Whether the message is a sub/resub/gift message |
| `flags.whisper` | Bool | Whether the message is a whisper |
| **Message** | | Actual message sent |
| `message.content` | String | Message content |
| `message.length` | Int | Message length |

*Note: To compare a `Color`, compare it to a color hex code string: `author.color == "#FF0000"`

### Data types
Generally, data types won't be much of an issue. However, mismatching datatypes can cause confusing results.

For example: 
- `"abc" + 123 == "abc123"` but `123 + "abc" != "123abc"` 
- `1 + "2" == 3` and `1 + "2" == "3"`. 

Chatterino will try to transform incompatible types for operations, but it isn't always successful.
If two types can't mix, `False` or `0` will be returned, depending on the context. 

Double check the table above to see the types of each variable to prevent unexpected results.

### About the order of operations
The order of operations in filters may not be exactly what you expect.

- Expressions in parenthesis are evaluated first
- Math operations are evaluated from left to right, not by MDAS. `2 + 3 * 4` yields `20`, not `14`.
- `a && b || c && d` is evaluated as `(a && b) || (c && d)`
- `a || b && c || d` is evaluated as `a || (b && c) || d`

Basically, if you're unsure about the order of operations, use extra parenthesis.
