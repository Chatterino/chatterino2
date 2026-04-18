// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include <sol/forward.hpp>

namespace chatterino::lua::api::emotes {

/* @lua-fragment

---@class c2.Emote
---@field name string
---@field images c2.ImageSet
---@field tooltip string
---@field home_page string URL to the platform specific emote page.
---@field zero_width boolean
---@field id string Platform specific ID.
---@field author string Username of the emote creator.
---@field base_name string|nil If this emote is aliased, this contains the original (base) name of the emote.

---Create a new emote. This emote is not cached anywhere.
---@param tbl {name: string, images: c2.ImageSet, tooltip: string, home_page?: string, zero_width?: boolean, id?: string, author?: string, base_name?: string}
---@return c2.Emote
function c2.Emote.new_uncached(tbl) end
*/

void createUserTypes(sol::table &c2);

}  // namespace chatterino::lua::api::emotes

#endif
