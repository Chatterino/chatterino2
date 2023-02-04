# Plugins

If Chatterino is compiled with the `CHATTERINO_PLUGINS` CMake option, it can
load and execute Lua files. Note that while there are attempts at making this
decently safe, we cannot guarantee safety.

## Plugin structure

Chatterino searches for plugins in the `Plugins` directory in the app data, right next to `Settings` and `Logs`.

Each plugin should have its own directory.

```
Chatterino dir/
└── plugin_name/
    ├── init.lua
    └── info.json
```

`init.lua` will be the file loaded when the plugin is enabled. You may load other files using `loadfile` Lua global function.

`info.json` contains metadata about the plugin, like its name, description,
authors, homepage link, tags, version, license name. The version field **must**
be [semver 2.0](https://semver.org/) compliant. The general idea of `info.json`
will not change however the exact contents probably will, for example with
permission system ideas.
Example file:

```json
{
  "name": "Test plugin",
  "description": "This plugin is for testing stuff.",
  "authors": "Mm2PL",
  "homepage": "https://github.com/Chatterino/Chatterino2",
  "tags": ["test"],
  "version": "0.0.0",
  "license": "MIT"
}
```

An example plugin is available at [https://github.com/Mm2PL/Chatterino-test-plugin](https://github.com/Mm2PL/Chatterino-test-plugin)

## API

The following parts of the Lua standard library are loaded:

- `_G` (most globals)
- `table`
- `string`
- `math`
- `utf8`

The official manual for them is available [here](https://www.lua.org/manual/5.4/manual.html#6).

### Chatterino API

All Chatterino functions are exposed in a global table called `c2`. The following functions are available

#### `register_command(name, handler)`

Registers a new command called `name` which when executed will call `handler`.
Returns `true` if everything went ok, `false` if there already exists another
command with this name.

Example:

```lua
function cmdWords(ctx)
    -- ctx contains:
    -- words - table of words supplied to the command including the trigger
    -- channelName - name of the channel the command is being run in
    c2.system_msg(ctx.channelName, "Words are: " .. table.concat(ctx.words, " "))
end

c2.register_command("/words", cmdWords)
```

Limitations/known issues:

- commands registered in functions, not in the global scope might not show up in the settings UI,
  rebuilding the window content caused by reloading another plugin will solve this
- spaces in command names aren't handled very well (https://github.com/Chatterino/chatterino2/issues/1517)

#### `send_msg(channel, text)`

Sends a message to `channel` with the specified text. Also executes commands.

Example:

```
function cmdShout(ctx)
    table.remove(ctx.words, 1)
    local output = table.concat(ctx.words, " ")
    c2.send_msg(ctx.channelName, string.upper(output))
end
c2.register_command("/shout", cmdShout)
```

Limitations/Known issues:

- it is possible to trigger your own Lua command with this causing a potentially infinite loop

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

This function behaves really similarity to Lua's `load`, however it does not allow for bytecode to be executed.
It achieves this by forcing all inputs to be encoded with `UTF-8`.

See [official documentation](https://www.lua.org/manual/5.4/manual.html#pdf-load)

#### `execfile(filename)`

This function mimics Lua's `dofile` however relative paths are relative to your plugin's directory.
You are restricted to loading files in your plugin's directory. You cannot load files with bytecode inside.

Example:

```
execfile("stuff.lua") -- executes Plugins/name/stuff.lua
execfile("./stuff.lua") -- executes Plugins/name/stuff.lua
execfile("../stuff.lua") -- tries to load Plugins/stuff.lua and errors
execfile("luac.out") -- tried to load Plugins/name/luac.out and errors because it contains non-utf8 data
```
