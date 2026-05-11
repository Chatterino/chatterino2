#include "controllers/plugins/api/ConnectionHandle.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include <sol/sol.hpp>

namespace chatterino::lua::api {

void ConnectionHandle::createUserType(sol::table &c2)
{
    c2.new_usertype<ConnectionHandle>(
        "ConnectionHandle", sol::no_constructor,  //
        "disconnect",
        [](ConnectionHandle &hdl) {
            auto locked = hdl.connection.lock();
            if (locked)
            {
                *locked = pajlada::Signals::ScopedConnection{};
            }
        },
        "block",
        [](ConnectionHandle &hdl) {
            if (auto locked = hdl.connection.lock())
            {
                locked->block();
            }
        },
        "unblock",
        [](ConnectionHandle &hdl) {
            if (auto locked = hdl.connection.lock())
            {
                locked->unblock();
            }
        },
        "is_blocked",
        [](ConnectionHandle &hdl) {
            if (auto locked = hdl.connection.lock())
            {
                return locked->isBlocked();
            }
            return false;
        },
        "is_connected",
        [](ConnectionHandle &hdl) {
            if (auto locked = hdl.connection.lock())
            {
                return locked->isConnected();
            }
            return false;
        });
}

}  // namespace chatterino::lua::api

#endif
