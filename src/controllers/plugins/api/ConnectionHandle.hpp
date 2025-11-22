#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include <pajlada/signals/scoped-connection.hpp>
#    include <sol/forward.hpp>

#    include <memory>

namespace chatterino::lua::api {

/* @lua-fragment

---@class c2.ConnectionHandle
---A handle to a connection. 
---This handle does not automatically disconnect a signal by itself.
---Destroying/closing it will not have any effect.
c2.ConnectionHandle = {}

---Disconnect the signal
function c2.ConnectionHandle:disconnect() end

---Block events on this connection
function c2.ConnectionHandle:block() end

---Unblock events on this connection
function c2.ConnectionHandle:unblock() end

---Is this connection currently blocked?
---@return boolean is_blocked
function c2.ConnectionHandle:is_blocked() end

---Is this connection still connected?
---@return boolean is_connected
function c2.ConnectionHandle:is_connected() end

*/
struct ConnectionHandle {
    std::weak_ptr<pajlada::Signals::ScopedConnection> connection;

    static void createUserType(sol::table &c2);
};

}  // namespace chatterino::lua::api

#endif
