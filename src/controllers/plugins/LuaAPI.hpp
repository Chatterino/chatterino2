#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

extern "C" {
#    include <lua.h>
}
#    include "controllers/plugins/LuaUtilities.hpp"

#    include <QString>

#    include <cassert>
#    include <memory>
#    include <vector>

struct lua_State;
namespace chatterino::lua::api {
// function names in this namespace reflect what's visible inside Lua and follow the lua naming scheme

// NOLINTBEGIN(readability-identifier-naming)
// Following functions are exposed in c2 table.

// Comments in this file are special, the docs/plugin-meta.lua file is generated from them
// All multiline comments will be added into that file. See scripts/make_luals_meta.py script for more info.

/**
 * @exposeenum c2.LogLevel
 */
// Represents "calls" to qCDebug, qCInfo ...
enum class LogLevel { Debug, Info, Warning, Critical };

/**
 * @exposeenum c2.EventType
 */
enum class EventType {
    CompletionRequested,
};

/**
 * @lua@class CommandContext
 * @lua@field words string[] The words typed when executing the command. For example `/foo bar baz` will result in `{"/foo", "bar", "baz"}`.
 * @lua@field channel Channel The channel the command was executed in.
 */

/**
 * @lua@class CompletionList
 */
struct CompletionList {
    /**
     * @lua@field values string[] The completions
     */
    std::vector<QString> values{};

    /**
     * @lua@field hide_others boolean Whether other completions from Chatterino should be hidden/ignored.
     */
    bool hideOthers{};
};

/**
 * @lua@class CompletionEvent
 */
struct CompletionEvent {
    /**
     * @lua@field query string The word being completed
     */
    QString query;
    /**
     * @lua@field full_text_content string Content of the text input
     */
    QString full_text_content;
    /**
     * @lua@field cursor_position integer Position of the cursor in the text input in unicode codepoints (not bytes)
     */
    int cursor_position{};
    /**
     * @lua@field is_first_word boolean True if this is the first word in the input
     */
    bool is_first_word{};
};

/**
 * @includefile common/Channel.hpp
 * @includefile controllers/plugins/api/ChannelRef.hpp
 */

/**
 * Registers a new command called `name` which when executed will call `handler`.
 *
 * @lua@param name string The name of the command.
 * @lua@param handler fun(ctx: CommandContext) The handler to be invoked when the command gets executed.
 * @lua@return boolean ok  Returns `true` if everything went ok, `false` if a command with this name exists.
 * @exposed c2.register_command
 */
int c2_register_command(lua_State *L);

/**
 * Registers a callback to be invoked when completions for a term are requested.
 *
 * @lua@param type "CompletionRequested"
 * @lua@param func fun(event: CompletionEvent): CompletionList The callback to be invoked.
 * @exposed c2.register_callback
 */
int c2_register_callback(lua_State *L);

/**
 * Writes a message to the Chatterino log.
 *
 * @lua@param level c2.LogLevel The desired level.
 * @lua@param ... any Values to log. Should be convertible to a string with `tostring()`.
 * @exposed c2.log
 */
int c2_log(lua_State *L);

/**
 * Calls callback around msec milliseconds later. Does not freeze Chatterino.
 *
 * @lua@param callback fun() The callback that will be called.
 * @lua@param msec number How long to wait.
 * @exposed c2.later
 */
int c2_later(lua_State *L);

// These ones are global
int g_load(lua_State *L);
int g_print(lua_State *L);
// NOLINTEND(readability-identifier-naming)

// This is for require() exposed as an element of package.searchers
int searcherAbsolute(lua_State *L);
int searcherRelative(lua_State *L);

// This is a fat pointer that allows us to type check values given to functions needing a userdata.
// Ensure ALL userdata given to Lua are a subclass of this! Otherwise we garbage as a pointer!
struct UserData {
    enum class Type { Channel };
    Type type;
    bool isWeak;
};

template <UserData::Type T, typename U>
struct WeakPtrUserData : public UserData {
    std::weak_ptr<U> target;

    WeakPtrUserData(std::weak_ptr<U> t)
        : UserData()
        , target(t)
    {
        this->type = T;
        this->isWeak = true;
    }

    static WeakPtrUserData<T, U> *create(lua_State *L, std::weak_ptr<U> target)
    {
        void *ptr = lua_newuserdata(L, sizeof(WeakPtrUserData<T, U>));
        return new (ptr) WeakPtrUserData<T, U>(target);
    }

    static WeakPtrUserData<T, U> *from(UserData *target)
    {
        if (!target->isWeak)
        {
            return nullptr;
        }
        if (target->type != T)
        {
            return nullptr;
        }
        return reinterpret_cast<WeakPtrUserData<T, U> *>(target);
    }

    static WeakPtrUserData<T, U> *from(void *target)
    {
        return from(reinterpret_cast<UserData *>(target));
    }

    static int destroy(lua_State *L)
    {
        auto self = WeakPtrUserData<T, U>::from(lua_touserdata(L, -1));
        // Note it is safe to only check the weakness of the pointer, as
        // std::weak_ptr seems to have identical representation regardless of
        // what it points to
        assert(self->isWeak);

        self->target.reset();
        lua_pop(L, 1);  // Lua deallocates the memory for full user data
        return 0;
    }
};

template <UserData::Type T, typename U>
struct SharedPtrUserData : public UserData {
    std::shared_ptr<U> target;

    SharedPtrUserData(std::shared_ptr<U> t)
        : UserData()
        , target(t)
    {
        this->type = T;
        this->isWeak = false;
    }

    static SharedPtrUserData<T, U> *create(lua_State *L,
                                           std::shared_ptr<U> target)
    {
        void *ptr = lua_newuserdata(L, sizeof(SharedPtrUserData<T, U>));
        return new (ptr) SharedPtrUserData<T, U>(target);
    }

    static SharedPtrUserData<T, U> *from(UserData *target)
    {
        if (target->isWeak)
        {
            return nullptr;
        }
        if (target->type != T)
        {
            return nullptr;
        }
        return reinterpret_cast<SharedPtrUserData<T, U> *>(target);
    }

    static SharedPtrUserData<T, U> *from(void *target)
    {
        return from(reinterpret_cast<UserData *>(target));
    }

    static int destroy(lua_State *L)
    {
        auto self = SharedPtrUserData<T, U>::from(lua_touserdata(L, -1));
        // Note it is safe to only check the weakness of the pointer, as
        // std::shared_ptr seems to have identical representation regardless of
        // what it points to
        assert(!self->isWeak);

        self->target.reset();
        lua_pop(L, 1);  // Lua deallocates the memory for full user data
        return 0;
    }
};

}  // namespace chatterino::lua::api

#endif
