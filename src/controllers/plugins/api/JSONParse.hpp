// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include <sol/forward.hpp>

namespace chatterino::lua::api {

/// Parse JSON from a string into a Lua value
///
/// Exposed in the 'json' package as `parse(string[, options]): any`.
/// It uses the C-Function signature, because we mostly use the raw Lua API in
/// the implementation.
int jsonParse(lua_State *L);

}  // namespace chatterino::lua::api

#endif
