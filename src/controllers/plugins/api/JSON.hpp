// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include <sol/forward.hpp>

namespace chatterino::lua::api {

/// Loads the 'json' module as a table.
///
/// Because `nullptr` is used as a sentinel for "null", this also adds a
/// `__tostring` method on lightuserdata.
sol::object loadJson(sol::state_view lua);

}  // namespace chatterino::lua::api

#endif
