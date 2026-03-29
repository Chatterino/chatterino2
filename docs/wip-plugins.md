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
    ├── info.json
    └── data/
        └── This is where your data/configs can be dumped
```

`init.lua` will be the file loaded when the plugin is enabled. You may load other files using [`require` global function](#requiremodname).

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
  "license": "MIT",
  "permissions": []
}
```

An example plugin is available at [https://github.com/Mm2PL/Chatterino-test-plugin](https://github.com/Mm2PL/Chatterino-test-plugin)

## Permissions

Plugins can have permissions associated to them. Unless otherwise noted functions don't require permissions.
These are the valid permissions:

### FilesystemRead

Allows the plugin to read from its data directory.

Example:

```json
{
  ...,
  "permissions": [
    {
      "type": "FilesystemRead"
    },
    ...
  ]
}
```

### FilesystemWrite

Allows the plugin to write to files and create files in its data directory.

Example:

```json
{
  ...,
  "permissions": [
    {
      "type": "FilesystemWrite"
    },
    ...
  ]
}
```

### Network

Allows the plugin to send HTTP requests.

Example:

```json
{
  ...,
  "permissions": [
    {
      "type": "Network"
    },
    ...
  ]
}
```

## Plugins with Typescript

If you prefer, you may use [TypescriptToLua](https://typescripttolua.github.io)
to typecheck your plugins. There is a `chatterino.d.ts` file describing the API
in this directory. However, this has several drawbacks like harder debugging at
runtime.

## LuaLS type definitions

Type definitions for LuaLS are available in
[the `/lua-meta` directory](./lua-meta). These are generated from [the C++
headers](../src/controllers/plugins/LuaAPI.hpp) of Chatterino using [a
script](../scripts/make_luals_meta.py).

## API

The following parts of the Lua standard library are loaded:

- `_G` (most globals)
- `io` - except `stdin`, `stdout`, `stderr`. Some functions require permissions.
- `math`
- `string`
- `table`
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
function cmd_words(ctx)
    -- ctx contains:
    -- words - table of words supplied to the command including the trigger
    -- channel - the channel the command is being run in
    ctx.channel:add_system_message("Words are: " .. table.concat(ctx.words, " "))
end

c2.register_command("/words", cmd_words)
```

Limitations/known issues:

- Commands registered in functions, not in the global scope might not show up in the settings UI,
  rebuilding the window content caused by reloading another plugin will solve this.
- Spaces in command names aren't handled very well (https://github.com/Chatterino/chatterino2/issues/1517).

#### `register_callback(c2.EventType.CompletionRequested, handler)`

Registers a callback (`handler`) to process completions. The callback takes a single table with the following entries:

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
    c2.EventType.CompletionRequested,
    function(event)
        if ("!join"):startswith(event.query) then
            ---@type CompletionList
            return { hide_others = true, values = { "!join" } }
        end
        ---@type CompletionList
        return { hide_others = false, values = {} }
    end
)
```

#### `current_account()`

Returns a `TwitchAccount` representing the current account.

#### `TwitchAccount`

Represents a Twitch account. There can only be one selected at a time but
unselected accounts stay valid. An account can become invalid if removed by the
user in the settings. Using an invalid account produces an error.

```lua
local acc = c2.current_account()

print(acc:is_valid()) -- true unless user removed account
```

##### `TwitchAccount:login()`

Returns the login name of the account. This string may only contain lowercase ASCII.

```lua
print(acc:login()) -- "mm2pl"
```

##### `TwitchAccount:id()`

Returns the Twitch user ID of the account. This uniquely and persistently identifies the account.

```lua
print(acc:id()) -- "117691339"
```

##### `TwitchAccount:color()`

Returns the color in chat of this account. If the user has not sent any
messages this will be `nil`.

```lua
print(acc:color()) -- "#ffdaa520"
```

##### `TwitchAccount:is_anon()`

Returns `true` if this account is an anonymous account (no associated Twitch user).

#### `ChannelType` enum

This table describes channel types Chatterino supports. The values behind the
names may change, do not count on them. It has the following keys:

- `None`
- `Direct`
- `Twitch`
- `TwitchWhispers`
- `TwitchWatching`
- `TwitchMentions`
- `TwitchLive`
- `TwitchAutomod`
- `TwitchEnd`
- `Irc`
- `Misc`

#### `Channel`

This is a type that represents a channel. Existence of this object doesn't
force Chatterino to hold the channel open. Should the user close the last split
holding this channel open, your Channel object will expire. You can check for
this using the `Channel:is_valid()` function. Using any other function on an
expired Channel yields an error. Using any `Channel` member function on a
non-`Channel` table also yields an error.

Some functions make sense only for Twitch channel, these yield an error when
used on non-Twitch channels. Special channels while marked as
`is_twitch_channel() = true` do not have these functions. To check if a channel
is an actual Twitch chatroom use `Channel:get_type()` instead of
`Channel:is_twitch_channel()`.

##### `Channel:by_name(name)`

Finds a channel given by `name`. Returns the channel or `nil` if not open.

Some miscellaneous channels are marked as if they are specifically Twitch channels:

- `/whispers`
- `/mentions`
- `/watching`
- `/live`
- `/automod`

Example:

```lua
local pajladas = c2.Channel.by_name("pajlada")
```

##### `Channel:by_twitch_id(id)`

Finds a channel given by the string representation of the owner's Twitch user ID. Returns the channel or `nil` if not open.

Example:

```lua
local pajladas = c2.Channel.by_twitch_id("11148817")
```

##### `Channel:get_name()`

On Twitch returns the lowercase login name of the channel owner. On IRC returns the normalized channel name.

Example:

```lua
-- Note: if the channel is not open this errors
pajladas:get_name()  -- "pajlada"
```

##### `Channel:get_type()`

Returns the channel's type. See [`ChannelType` enum](#ChannelType-enum).

##### `Channel:get_display_name()`

Returns the channel owner's display name. This can contain characters that are not lowercase and even non-ASCII.

Example:

```lua
local saddummys = c2.Channel.by_name("saddummy")
saddummys:get_display_name() -- "서새봄냥"
```

<!-- F Korean Twitch, apparently you were not profitable enough -->

##### `Channel:on_display_name_changed(cb)`

Callback when the channel display name changes. The callback doesn't get any
arguments, use [`Channel:get_display_name`](#channelget_display_name) to get the
updated name.

This returns a [`ConnectionHandle`](#connectionhandle) which can be used to
disconnect the handler.

##### `Channel:send_message(message[, execute_commands])`

Sends a message to the channel with the given text. If `execute_commands` is
not present or `false` commands will not be executed client-side, this affects
all user commands and all Twitch commands except `/me`.

Examples:

```lua
-- times out @Mm2PL
pajladas:send_message("/timeout mm2pl 1s test", true)

-- results in a "Unknown command" error from Twitch
pajladas:send_message("/timeout mm2pl 1s test")

-- Given a user command "hello":
-- this will execute it
pajladas:send_message("hello", true)
-- this will send "hello" literally, bypassing commands
pajladas:send_message("hello")

function cmd_shout(ctx)
    table.remove(ctx.words, 1)
    local output = table.concat(ctx.words, " ")
    ctx.channel:send_message(string.upper(output))
end
c2.register_command("/shout", cmd_shout)
```

Limitations/Known issues:

- It is possible to trigger your own Lua command with this causing a potentially infinite loop.

##### `Channel:add_system_message(message)`

Shows a system message in the channel with the given text.

Example:

```lua
pajladas:add_system_message("Hello, world!")
```

#### `Channel:add_message(message[, context[, override_flags]])`

Add a rich message to a channel. The message can be created with [`Message.new`](#messagenewdata).

Example:

```lua
channel:add_message(c2.Message.new({
    id = "myplugin-1234",
    elements = {
        {
            type = "text",
            text = "Hello, World!",
        }
    }
}))
```

##### `Channel:is_twitch_channel()`

Returns `true` if the channel is a Twitch channel, that is its type name has
the `Twitch` prefix. This returns `true` for special channels like Mentions.
You might want `Channel:get_type() == c2.ChannelType.Twitch` if you want to use
Twitch-specific functions.

##### `Channel:get_twitch_id()`

Returns the string form of the channel owner's Twitch user ID.

Example:

```lua
pajladas:get_twitch_id() -- "11148817"
```

##### `Channel:is_broadcaster()`

Returns `true` if the channel is owned by the current user.

##### `Channel:is_mod()`

Returns `true` if the channel can be moderated by the current user.

##### `Channel:is_vip()`

Returns `true` if the current user is a VIP in the channel.

##### `Channel:message_snapshot(n_items)`

Get a list of messages in this channel (starting from the most recent messages).
The snapshot is returned as a usertype that wraps a C++ object.
`n_items` is an upper bound, the actual number of messages returned might be lower.

##### `Channel:last_message()`

Get the most recent message. If this channel doesn't have any message, this returns `nil`.

##### `Channel:replace_message(message, replacement[, hint])`

Replace a specific message with a different one.
`hint` is a one-based index (from the start) where the message is probably located. This is checked first. Otherwise the behavior is identical to the overload without this parameter.

##### `Channel:replace_message_at(index, replacement)`

Replace a message at an index with a different one.
`index` is a one-based index (from the start) of the message to replace.

##### `Channel:clear_messages()`

Remove all messages in this channel.

##### `Channel:find_message_by_id(id)`

Find a message by its ID. If no message is found, `nil` is returned.

##### `Channel:has_messages()`

Check if the channel has any messages.

##### `Channel:count_messages()`

Count the number of messages in this channel.

#### `HTTPMethod` enum

This table describes HTTP methods available to Lua Plugins. The values behind
the names may change, do not count on them. It has the following keys:

- `Get`
- `Post`
- `Put`
- `Delete`
- `Patch`

#### `HTTPResponse`

An `HTTPResponse` is a table you receive in the callback after a completed `HTTPRequest`.

##### `HTTPResponse.data()`

This function returns the data received from the server as a string. Usually
this will be UTF-8-encoded however that is not guaranteed, this could be any
binary data.

##### `HTTPResponse.error()`

If an error happened this function returns a human readable description of it.

It returns something like: `"ConnectionRefusedError"`, `"401"`.

##### `HTTPResponse.status()`

This function returns the HTTP status code of the request or `nil` if there was
an error before a status code could be received.

#### `HTTPRequest`

Allows you to send an HTTP request to a URL. Do not create requests that you
don't want to call `execute()` on. For the time being that leaks callback
functions and all their upvalues with them.

##### `HTTPRequest.create(method, url)`

Creates a new `HTTPRequest`. The `method` argument is an
[`HTTPMethod`](#HTTPMethod-enum). The `url` argument must be a string
containing a valid URL (ex. `https://example.com/path/to/api`).

```lua
local req = c2.HTTPRequest.create(c2.HTTPMethod.Get, "https://example.com")
req:on_success(function (res)
    print(res:data())
end)
req:execute()
```

##### `HTTPRequest:on_success(callback)`

Sets the success callback. It accepts a function that takes a single parameter
of type `HTTPResponse`. The callback will be called on success. This function
returns nothing.

##### `HTTPRequest:on_error(callback)`

Sets the error callback. It accepts a function that takes a single parameter of
type `HTTPResponse`. The callback will be called if the request fails. To see why
it failed check the `error` field of the result. This function returns nothing.

##### `HTTPRequest:finally(callback)`

Sets the finally callback. It accepts a function that takes no parameters and
returns nothing. It will be always called after `success` or `error`. This
function returns nothing.

##### `HTTPRequest:set_timeout(timeout)`

Sets how long the request will take before it times out. The `timeout`
parameter is in milliseconds. This function returns nothing.

##### `HTTPRequest:set_payload(data)`

Sets the data that will be sent with the request. The `data` parameter must be
a string. This function returns nothing.

##### `HTTPRequest:set_header(name, value)`

Adds or overwrites a header in the request. Both `name` and `value` should be
strings. If they are not strings they will be converted to strings. This
function returns nothing.

##### `HTTPRequest:execute()`

Sends the request. This function returns nothing.

```lua
local url = "http://localhost:8080/thing"
local request = c2.HTTPRequest.create(c2.HTTPMethod.Post, url)
request:set_timeout(1000)
request:set_payload("TEST!")
request:set_header("X-Test", "Testing!")
request:set_header("Content-Type", "text/plain")
request:on_success(function (res)
    print('Success!')
    -- Data is in res.data
    print(res:status())
end)
request:on_error(function (res)
    print('Error!')
    print(res:status())
    print(res:error())
end)
request:finally(function ()
    print('Finally')
end)
request:execute()

-- This prints:
-- Success!
-- [content of /thing]
-- 200
-- Finally

-- Or:
-- Error!
-- nil
-- ConnectionRefusedError
```

#### `WebSocket`

This API allows you to connect to WebSocket servers. For example, you can do the following:

```lua
local ws = c2.WebSocket.new("wss://echo.websocket.org",
  -- this object is optional
  {
    on_open = function()
      print("Connection established")
    end,
    on_close = function()
      print("Connection closed")
    end
  }
)
-- handlers can be passed in the constructor or set here
ws.on_text = function(data)
  print("Got text: " .. data)
end

ws:send_text("Hello, World!")
```

##### `WebSocket.new(url[, options])`

Create and connect to a WebSocket server specified by `url`.

`options`, if specified, has to be a table with the following members (all optional):

| Key         | Type                    | Description                                                                                                                             |
| ----------- | ----------------------- | --------------------------------------------------------------------------------------------------------------------------------------- |
| `headers`   | `table<string, string>` | Additional headers to set when connecting to the server. Any headers specified here will overwrite the ones Chatterino sets by default. |
| `on_open`   | `fun()`                 | Called when the WebSocket handshake completed.                                                                                          |
| `on_text`   | `fun(data: string)`     | Handler for text messages.                                                                                                              |
| `on_binary` | `fun(data: string)`     | Handler for binary messages. Here, the data might not be valid UTF-8.                                                                   |
| `on_close`  | `fun()`                 | Handler for a close event. This handler is also called if the connection failed.                                                        |

The returned object has writable properties for the `on_close`, `on_text`, `on_binary`, and `on_open` handlers.

##### `WebSocket:send_text(data)`

Sends a text messsage to the server. If the socket is not yet connected, this message is queued and sent once a connection is established.

##### `WebSocket:send_binary(data)`

Sends a binary message to the server. If the socket is not yet connected, this message is queued and sent once a connection is established.

##### `WebSocket:close()`

Closes the WebSocket connection.

#### `Message`

Allows creation and modification of rich chat messages. A `Message` is
Chatterino's representation of a chat message or any system message.
The interface to Lua is currently limited but is expected to be expanded soon.

Messages can be added to a [`Channel`](#channel). Once a message is added to a
channel, it can't be modified anymore (except for its `flags`). These messages
are termed "frozen" (immutable). You can check for this using the `frozen`
property:

```lua
local my_msg = c2.Message.new({ id = "foobar" })
assert(not my_msg.frozen)
my_channel:add_message(my_msg)
assert(my_msg.frozen)
```

A message has the same properties that it takes in its constructor (e.g.
`msg.id` or `msg.flags`). If the message is not frozen (mutable), these
properties can be modified. New elements can be added with
[`append_element`](#messageappend_elementdata).

##### `Message.new(data)`

Creates a new message from a table. The message can be added to a channel using
[`Channel:add_message`](#channeladd_messagemessage-context-override_flags):

```lua
c2.register_command("/testing", function(ctx)
    ctx.channel:add_message(c2.Message.new({
        id = "myplugin-1234",
        highlight_color = "#80ff0000",
        flags = c2.MessageFlag.Highlighted,
        elements = {
            {
                type = "text",
                color = "link",
                text = "Hover me!",
                tooltip = "<h1>This is text from my plugin</h1>"
            },
            {
                type = "mention",
                display_name = "@User",
                login_name = "twitchdev",
                fallback_color = "text",
                user_color = "blue",
            }
        }
    }))
end)
```

The full range of options can be found in the typing files ([LuaLS](./lua-meta/globals.lua), [TypeScript](./chatterino.d.ts)).

##### `Message:elements()`

Gets a reference to the message's elements. This can be indexed or iterated
though. Through the return value, you can also remove elements. Note that
currently, new elements can't be added here (use `Message:append_element`).

The return value can be thought of as a table of `MessageElement`s.

```lua
local my_msg = c2.Message.new({
  id = "foo",
  elements = {
    { type = "text", text = "foo" },
    { type = "text", text = "bar" },
    { type = "text", text = "baz" },
  }
})
local elements = my_msg:elements()
assert(#elements == 3)
assert(elements[1].words[1] == "foo")
assert(elements[2].words[1] == "bar")

elements:erase(2) -- erase "bar"
assert(#elements == 2)
assert(elements[1].words[1] == "foo")
assert(elements[2].words[1] == "baz")
```

##### `Message:append_element(data)`

Creates a message element from a table (`data`) and appends it to the message.
The structure of the table matches the one taken in `Message.new`'s `elements`.

```lua
local my_msg = c2.Message.new({ id = "foo" })
my_msg:append_element({
  type = "text",
  text = "My text element",
})
```

#### `MessageElement`

A reference to an element inside a `Message` (essentially an in index into the
elements). Note that modifications to the elements of the parent `Message` might
change the actual element this is referring to.

To distinguish different types of elements, check the `type` property. This
takes the same values as `element.type` in the `Message` constructor.

The full range of properties can be found in the typing files ([LuaLS](./lua-meta/globals.lua), [TypeScript](./chatterino.d.ts)).

#### `LinkType` enum

This table describes links available to plugins.

| `LinkType`        | `c2.Link.value` content            | Action on click                                                                      | Example                               |
| ----------------- | ---------------------------------- | ------------------------------------------------------------------------------------ | ------------------------------------- |
| `Url`             | Any URI that makes sense to open   | Open Link in browser                                                                 | `https://example.org`                 |
| `UserInfo`        | A Twitch username or `id:TwitchID` | Open a usercard                                                                      | `mm2pl`, `id:117691339`               |
| `UserAction`      | Command to run or message to send  | Send command/message                                                                 | `/timeout mm2pl 1s test`, `!spoilers` |
| `JumpToChannel`   | [Channel name](#channelget_name)   | Go to already open split with given channel                                          | `#pajlada`                            |
| `CopyToClipboard` | Any Unicode text                   | Copy value to clipboard                                                              | n/a                                   |
| `JumpToMessage`   | ID of the message                  | Highlight the message with given ID in current split, do nothing if it was not found | n/a                                   |
| `InsertText`      | Any text, command or emote         | Insert text into split input                                                         | n/a                                   |

#### `ConnectionHandle`

This type represents a handle to a registration of a callback for an event handler.
Conceptually, the event has a _connection_ to the callback/handler.
This handle can be used to modify that connection.
It does not automatically disconnect the connection when it's destroyed (in `__gc`) -
[`disconnect()`](#connectionhandledisconnect) has to be called manually.

##### `ConnectionHandle:disconnect()`

Disconnect the signal.

##### `ConnectionHandle:block()`

Block events on this connection.

##### `ConnectionHandle:unblock()`

Unblock events on this connection.

##### `ConnectionHandle:is_blocked()`

Is this connection currently blocked?

##### `ConnectionHandle:is_connected()`

Is this connection still connected?

### Input/Output API

These functions are wrappers for Lua's I/O library. Functions on file pointer
objects (`FILE*`) are not modified or replaced. [You can read the documentation
for them here](https://www.lua.org/manual/5.4/manual.html#pdf-file:close).
Chatterino does _not_ give you stdin and stdout as default input and output
respectively. The following objects are missing from the `io` table exposed by
Chatterino compared to Lua's native library: `stdin`, `stdout`, `stderr`.

#### `close([file])`

Closes a file. If not given, `io.output()` is used instead.

See [official documentation](https://www.lua.org/manual/5.4/manual.html#pdf-io.close)

#### `flush()`

Flushes `io.output()`.

See [official documentation](https://www.lua.org/manual/5.4/manual.html#pdf-io.flush)

#### `input([file_or_name])`

When called with no arguments this function returns the default input file.
This variant requires no permissions.

When called with a file object, it will set the default input file to the one
given. This one also requires no permissions.

When called with a filename as a string, it will open that file for reading.
Equivalent to: `io.input(io.open(filename))`. This variant requires
the `FilesystemRead` permission and the given file to be within the plugin's
data directory.

See [official documentation](https://www.lua.org/manual/5.4/manual.html#pdf-io.input)

#### `lines([filename, ...])`

With no arguments this function is equivalent to `io.input():lines("l")`. See
[Lua documentation for file:flush()](https://www.lua.org/manual/5.4/manual.html#pdf-file:flush).
This variant requires no permissions.

With `filename` given it is most like `io.open(filename):lines(...)`. This
variant requires the `FilesystemRead` permission and the given file to be
within the plugin's data directory.

See [official documentation](https://www.lua.org/manual/5.4/manual.html#pdf-io.lines)

#### `open(filename [, mode])`

This functions opens the given file with a mode. It requires `filename` to be
within the plugin's data directory. A call with no mode given is equivalent to
one with `mode="r"`.
Depending on the mode this function has slightly different behavior:

| Mode        | Permission        | Read? | Write? | Truncate? | Create? |
| ----------- | ----------------- | ----- | ------ | --------- | ------- |
| `r` read    | `FilesystemRead`  | Yes   | No     | No        | No      |
| `w` write   | `FilesystemWrite` | No    | Yes    | Yes       | Yes     |
| `a` append  | `FilesystemWrite` | No    | Append | No        | Yes     |
| `r+` update | `FilesystemWrite` | Yes   | Yes    | No        | No      |
| `w+` update | `FilesystemWrite` | Yes   | Yes    | Yes       | Yes     |
| `a+` update | `FilesystemWrite` | Yes   | Append | No        | Yes     |

To open a file in binary mode add a `b` at the end of the mode.

See [official documentation](https://www.lua.org/manual/5.4/manual.html#pdf-io.open)

#### `output([file_or_name])`

This is identical to [`io.input()`](#inputfile_or_name) but operates on the
default output and opens the file in write mode instead. Requires
`FilesystemWrite` instead of `FilesystemRead`.

See [official documentation](https://www.lua.org/manual/5.4/manual.html#pdf-io.output)

#### `popen(exe [, mode])`

This function is unavailable in Chatterino. Calling it results in an error
message to let you know that it's not available, no permissions needed.

See [official documentation](https://www.lua.org/manual/5.4/manual.html#pdf-io.popen)

#### `read(...)`

Equivalent to `io.input():read(...)`. See [`io.input()`](#inputfile_or_name)
and [`file:read()`](https://www.lua.org/manual/5.4/manual.html#pdf-file:read).

See [official documentation](https://www.lua.org/manual/5.4/manual.html#pdf-io.read)

#### `tmpfile()`

This function is unavailable in Chatterino. Calling it results in an error
message to let you know that it's not available, no permissions needed.

See [official documentation](https://www.lua.org/manual/5.4/manual.html#pdf-io.tmpfile)

#### `type(obj)`

This functions allows you to tell if the object is a `file`, a `closed file` or
a different bit of data.

See [official documentation](https://www.lua.org/manual/5.4/manual.html#pdf-io.type)

#### `write(...)`

Equivalent to `io.output():write(...)`. See [`io.output()`](#outputfile_or_name)
and [`file:write()`](https://www.lua.org/manual/5.4/manual.html#pdf-file:write).

See [official documentation](https://www.lua.org/manual/5.4/manual.html#pdf-io.write)

### Debug API

To aid debugging, Chatterino provides Lua's `debug.traceback` function.
Other functions from the `debug` library are not exposed.

#### `traceback([thread,] [message [, level]])`

Returns a stack trace of `thread` or the current thread with an optional `message` prepended.
`level` can be used to skip some frames. By default, it's 1 (skipping the current function).

This can be used as a message handler in `xpcall`:

```lua
local function main()
  c2.ThisDoesNotExistAndWillError()
end

local ok, result = xpcall(main, debug.traceback)
if not ok then
  print(result)
end
```

See [official documentation](https://www.lua.org/manual/5.4/manual.html#pdf-debug.traceback)

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
- binary chunks are never loaded,
- files inside of the plugin `data` directory are never loaded

As in normal Lua, dots are converted to the path separators (`'/'` on Linux and Mac, `'\'` on Windows).

Example:

```lua
require("stuff") -- executes Plugins/name/stuff.lua or $(dirname $CURR_FILE)/stuff.lua
require("dir.name") -- executes Plugins/name/dir/name.lua or $(dirname $CURR_FILE)/dir/name.lua
require("binary") -- tried to load Plugins/name/binary.lua and errors because binary is not a text file
require("data.file") -- tried to load Plugins/name/data/file.lua and errors because that is not allowed
```

#### `print(Args...)`

The `print` global function is equivalent to calling `c2.log(c2.LogLevel.Debug, Args...)`

### JSON API

Chatterino includes the `chatterino.json` module for parsing and serializing JSON:

```lua
local json = require('chatterino.json')

local parsed = json.parse('{"foo": 1}')
-- { foo = 1 }

local str = json.stringify({ foo = 1 })
-- '{"foo":1}'
```

#### `parse(string[, options])`

Parse a string as JSON. Errors if the input was invalid JSON. Use [`pcall`](https://www.lua.org/pil/8.4.html) when parsing untrusted/user input.

```lua
local json = require('chatterino.json')

local parsed = json.parse('{"foo": 1}')
-- { foo = 1 }

local ok, result = pcall(json.parse, 'invalid input')
-- ok = false, result = "Failed to parse JSON: syntax error..."

local ok, result = pcall(json.parse, '{"foo": 1 /* foo */ }', { allow_comments = true })
-- ok = true, result = { foo = 1 }
```

`options` can be an optional table with the following optional keys:

- `allow_comments` (boolean): Allow C++ style comments (`/* foo */` and `// foo`)
- `allow_trailing_commas` (boolean): Allow trailing comments in objects and arrays (`[1, 2,]`)

#### `stringify(value[, options])`

Stringify a Lua value as JSON. Only tables and scalars (strings/numbers/booleans) are supported.
Empty tables are stringified as objects. To get an empty array, use the following: `{ [0] = json.null }` (will produce `[]`).
Tables with `nil` values like `{ foo = nil }` will be stringified as `{}` (they are identical to the empty table).
To get `null` there, use `json.null`.

```lua
local json = require('chatterino.json')

local str = json.stringify({ foo = 1, bar = nil, baz = json.null })
-- '{"foo":1,"baz":null}'

local str = json.stringify({ foo = 1 }, { pretty = true })
-- {
--     "foo": 1
-- }
```

`options` can be an optional table with the following optional keys:

- `pretty` (boolean): Use newlines and indentation when stringifying
- `indent_char` (string, default: space): Character to use when indenting object/array items
- `indent_size` (number, default: 4): Amount of times `indent_char` is repeated per nesting-level

#### `null`

A sentinel to indicate a `null` value.
This is useful if `nil` would hide the value (such as in tables):

```lua
local json = require('chatterino.json')

local str = json.stringify({ foo = 1, bar = nil, baz = json.null })
-- '{"foo":1,"baz":null}'

local obj = json.parse(str)
assert(obj.baz == json.null)
assert(obj.baz ~= nil)
```
