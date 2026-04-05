// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include <sol/forward.hpp>

namespace chatterino::lua::api {

/// Serializes a Lua value to JSON.
///
/// Exposed in the 'json' package as `stringify(value[, options]): string`.
/// It uses the C-Function signature, because we mostly use the raw Lua API in
/// the implementation.
int jsonStringify(lua_State *L);

}  // namespace chatterino::lua::api

#endif
