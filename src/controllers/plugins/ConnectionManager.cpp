#include "controllers/plugins/ConnectionManager.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include <algorithm>

namespace chatterino::lua {

api::ConnectionHandle ConnectionManager::add(
    pajlada::Signals::ScopedConnection &&conn)
{
    this->removeDisconnected();
    return {
        .connection = this->connections.emplace_back(
            std::make_shared<pajlada::Signals::ScopedConnection>(
                std::move(conn))),
    };
}

void ConnectionManager::clear()
{
    this->connections.clear();
}

void ConnectionManager::removeDisconnected()
{
    auto [first, last] =
        std::ranges::remove_if(this->connections, [](const auto &conn) {
            return !conn || !conn->isConnected();
        });
    this->connections.erase(first, last);
}

}  // namespace chatterino::lua

#endif
