// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include <sol/forward.hpp>

namespace chatterino::lua::api {

/// A reimplementation of Lua's debug.traceback (ldblib.c).
/// Creates a traceback of a thread.
/// It's intended to be used as a message handler in `xpcall`.
/// See https://www.lua.org/manual/5.4/manual.html#pdf-debug.traceback
int debugTraceback(lua_State *L);

}  // namespace chatterino::lua::api
#endif
