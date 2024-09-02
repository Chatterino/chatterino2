# Lua wrapper

The `chatterino::lua` namespace contains a wrapper for most used Lua functions.
Most of them live in `src/controllers/plugins/LuaUtilities.hpp`.

- When doing any kind of complex work with the Lua stack, use
  `lua::StackGuard`, which will abort the program should the stack size not be
  what you expect.
- When possible use `lua::push(lua_State *, T)`,
 `lua::peek(lua_State*, T*, StackIdx)` or `lua::pop(lua_State *, T*)` instead
 of Lua's APIs directly.

# Error paths

When you create error paths that call `luaL_error` or `lua_error` directly, you
should be careful of local variables. Consider this:

```cpp
// XXX: INCORRECT
void Foo::frobulate(lua_State *L)
{
    // stack size check omitted

    QString somestring;
    if (!lua::pop(L, &somestring))
    {
        return luaL_error(L, "Foo:frobulate failed to get somestring (1st argument), expected a string");
    }
    int someint;
    if (!lua::pop(L, &someint))
    {
        // Error path 2
        return luaL_error(L, "Foo:frobulate failed to get somesint (2nd argument), expected an integer");
    }
    // ...
}
```

If `Foo:frobulate()` gets called with a valid string but an invalid int
argument _error path 2_ is invoked. This means that the function returns
without ever calling `QString::~QString()` on `somestring` which leaks memory
that object owns. To solve this you can use the `drop()` utility function to
call destructor of your object:

```cpp
void Foo::frobulate(lua_State *L)
{
    // stack size check omitted

    QString somestring;
    if (!lua::pop(L, &somestring))
    {
        drop(somestring);
        return luaL_error(L, "Foo:frobulate failed to get somestring (1st argument), expected a string");
    }
    int someint;
    if (!lua::pop(L, &someint))
    {
        // Error path 2
        drop(somestring);
        return luaL_error(L, "Foo:frobulate failed to get somesint (2nd argument), expected an integer");
    }
    // ...
}
```

This code example properly deallocates all memory held by local variables. Note
that you should not use `drop()` on trivial objects that is objects that do not
need any explicitly created destructor.
