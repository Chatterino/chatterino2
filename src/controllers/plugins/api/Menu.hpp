// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include <sol/forward.hpp>

namespace chatterino::lua::api::menu {

/* @lua-fragment
---A generic menu used for context menus.
---@class c2.Menu
c2.Menu = {}

---Appends a new action to the menu.
---@param text string
---@param cb fun()
function c2.Menu:add_action(text, cb) end

---Inserts an action named `text` after `before`. If `before` is not found,
---the action is inserted at the end. `before` can either be a name or a 
---one-based index.
---@param before string|integer A name or index of an action.
---@param text string
---@param cb fun()
function c2.Menu:insert_action(before, text, cb) end

---Appends a new Menu with `title` to the menu.
---@param title string
---@return c2.Menu
function c2.Menu:add_menu(title) end

---Inserts a new Menu named `title` after `before`. If `before` is not found,
---the menu is inserted at the end. `before` can either be a name or a one-based
---index.
---@param before string|integer A name or index of an action.
---@param title string
function c2.Menu:insert_menu(before, title) end

---Appends a new separator.
function c2.Menu:add_separator() end

---Inserts a new separator after `before`. If `before` is not found,
---the separator is inserted at the end. `before` can either be a name or a 
---one-based index.
---@param before string|integer A name or index of an action.
function c2.Menu:insert_separator(before) end
*/

/// Creates the c2.Menu user type
void createUserType(sol::table &c2);

}  // namespace chatterino::lua::api::menu

#endif
