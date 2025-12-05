#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include <sol/forward.hpp>

namespace chatterino::lua::api {

/* @lua-fragment
---@class c2.TwitchAccount
c2.TwitchAccount = {}

--- Returns true if the account this object points to is valid.
--- If the object expired, returns false
---
---@return boolean success
function c2.TwitchAccount:is_valid() end

---@return string user_login The (login) name of the account
function c2.TwitchAccount:login() end

---@return string user_id The Twitch user ID of the account
function c2.TwitchAccount:id() end

---@return string? color Color in chat of this account. `nil` if not yet known
function c2.TwitchAccount:color() end

---@return boolean is_anon `true` if this account is an anonymous account (no associated Twitch user)
function c2.TwitchAccount:is_anon() end

---@return string str
function c2.TwitchAccount:__tostring() end

---Gets the currently logged in Twitch account. This account might be an anonymous one (see `is_anon`).
---@return c2.TwitchAccount account
function c2.current_account() end
*/
void createAccounts(sol::table &c2);

}  // namespace chatterino::lua::api

#endif
