# Plugins

If Chatterino is compiled with the `CHATTERINO_PLUGINS` CMake option, it can
load and execute Lua files. Note that while there are attempts at making this
decently safe, we cannot guarantee safety.

## Plugin structure

Chatterino searches for plugins in the `Plugins` directory in the app data, right next to `Settings` and `Logs`.

Each plugin should have its own directory.

```
Chatterino Plugins dir/
└── plugin_name/
    ├── init.lua
    └── info.json
```

`init.lua` will be the file loaded when the plugin is enabled. You may load other files using [`import` global function](#importfilename=).

`info.json` contains metadata about the plugin, like its name, description,
authors, homepage link, tags, version, license name. The version field **must**
be [semver 2.0](https://semver.org/) compliant. The general idea of `info.json`
will not change however the exact contents probably will, for example with
permission system ideas.
Example file:

```json
{
  "$schema": "https://raw.githubusercontent.com/Chatterino/chatterino2/master/docs/plugin-info.schema.json",
  "name": "Test plugin",
  "description": "This plugin is for testing stuff.",
  "authors": ["Mm2PL"],
  "homepage": "https://github.com/Chatterino/Chatterino2",
  "tags": ["test"],
  "version": "0.0.0",
  "license": "MIT"
}
```

An example plugin is available at [https://github.com/Mm2PL/Chatterino-test-plugin](https://github.com/Mm2PL/Chatterino-test-plugin)

## Plugins with Typescript

If you prefer, you may use [TypescriptToLua](https://typescripttolua.github.io)
to typecheck your plugins. There is a `chatterino.d.ts` file describing the API
in this directory. However, this has several drawbacks like harder debugging at
runtime.

## API

The following parts of the Lua standard library are loaded:

- `_G` (most globals)
- `table`
- `string`
- `math`
- `utf8`

The official manual for them is available [here](https://www.lua.org/manual/5.4/manual.html#6).

### Chatterino API

All Chatterino functions are exposed in a global table called `c2`. The following members are available:

#### `log(level, args...)`

Writes a message to the Chatterino log. The `level` argument should be a
`LogLevel` member. All `args` should be convertible to a string with
`tostring()`.

Example:

```lua
c2.log(c2.LogLevel.Warning, "Hello, this should show up in the Chatterino log by default")

c2.log(c2.LogLevel.Debug, "Hello world")
-- Equivalent to doing qCDebug(chatterinoLua) << "[pluginDirectory:Plugin Name]" << "Hello, world"; from C++
```

#### `LogLevel` enum

This table describes log levels available to Lua Plugins. The values behind the names may change, do not count on them. It has the following keys:

- `Debug`
- `Info`
- `Warning`
- `Critical`

#### `register_command(name, handler)`

Registers a new command called `name` which when executed will call `handler`.
Returns `true` if everything went ok, `false` if there already exists another
command with this name.

Example:

```lua
function cmdWords(ctx)
    -- ctx contains:
    -- words - table of words supplied to the command including the trigger
    -- channel_name - name of the channel the command is being run in
    c2.system_msg(ctx.channel_name, "Words are: " .. table.concat(ctx.words, " "))
end

c2.register_command("/words", cmdWords)
```

Limitations/known issues:

- Commands registered in functions, not in the global scope might not show up in the settings UI,
  rebuilding the window content caused by reloading another plugin will solve this.
- Spaces in command names aren't handled very well (https://github.com/Chatterino/chatterino2/issues/1517).

#### `register_callback("CompletionRequested", handler)`

Registers a callback (`handler`) to process completions. The callback gets the following parameters:

- `query`: The queried word.
- `full_text_content`: The whole input.
- `cursor_position`: The position of the cursor in the input.
- `is_first_word`: Flag whether `query` is the first word in the input.

Example:

| Input      | `query` | `full_text_content` | `cursor_position` | `is_first_word` |
| ---------- | ------- | ------------------- | ----------------- | --------------- |
| `foo│`     | `foo`   | `foo`               | 3                 | `true`          |
| `fo│o`     | `fo`    | `foo`               | 2                 | `true`          |
| `foo bar│` | `bar`   | `foo bar`           | 7                 | `false`         |
| `foo │bar` | `foo`   | `foo bar`           | 4                 | `false`         |

```lua
function string.startswith(s, other)
    return string.sub(s, 1, string.len(other)) == other
end

c2.register_callback(
    "CompletionRequested",
    function(query, full_text_content, cursor_position, is_first_word)
        if ("!join"):startswith(query) then
            ---@type CompletionList
            return { hide_others = true, values = { "!join" } }
        end
        ---@type CompletionList
        return { hide_others = false, values = {} }
    end
)
```

#### `send_msg(channel, text)`

Sends a message to `channel` with the specified text. Also executes commands.

Example:

```lua
function cmdShout(ctx)
    table.remove(ctx.words, 1)
    local output = table.concat(ctx.words, " ")
    c2.send_msg(ctx.channel_name, string.upper(output))
end
c2.register_command("/shout", cmdShout)
```

Limitations/Known issues:

- It is possible to trigger your own Lua command with this causing a potentially infinite loop.

#### `system_msg(channel, text)`

Creates a system message and adds it to the twitch channel specified by
`channel`. Returns `true` if everything went ok, `false` otherwise. It will
throw an error if the number of arguments received doesn't match what it
expects.

Example:

```lua
local ok = c2.system_msg("pajlada", "test")
if (not ok)
    -- channel not found
end
```

### Changed globals

#### `load(chunk [, chunkname [, mode [, env]]])`

This function is only available if Chatterino is compiled in debug mode. It is meant for debugging with little exception.
This function behaves really similarity to Lua's `load`, however it does not allow for bytecode to be executed.
It achieves this by forcing all inputs to be encoded with `UTF-8`.

See [official documentation](https://www.lua.org/manual/5.4/manual.html#pdf-load)

#### `require(modname)`

This is Lua's [`require()`](https://www.lua.org/manual/5.3/manual.html#pdf-require) function.
However, the searcher and load configuration is notably different from the default:

- Lua's built-in dynamic library searcher is removed,
- `package.path` is not used, in its place are two searchers,
- when `require()` is used, first a file relative to the currently executing
  file will be checked, then a file relative to the plugin directory,
- binary chunks are never loaded

As in normal Lua, dots are converted to the path separators (`'/'` on Linux and Mac, `'\'` on Windows).

Example:

```lua
require("stuff") -- executes Plugins/name/stuff.lua or $(dirname $CURR_FILE)/stuff.lua
require("dir.name") -- executes Plugins/name/dir/name.lua or $(dirname $CURR_FILE)/dir/name.lua
require("binary") -- tried to load Plugins/name/binary.lua and errors because binary is not a text file
```

#### `print(Args...)`

The `print` global function is equivalent to calling `c2.log(c2.LogLevel.Debug, Args...)`
