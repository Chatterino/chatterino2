#include "ConnectionOwner.hpp"

#include <QObject>

namespace ab
{
    ConnectionOwner::ConnectionOwner()
    {
    }

    ConnectionOwner::~ConnectionOwner()
    {
        this->clear();
    }

    ConnectionOwner& ConnectionOwner::operator+=(
        QMetaObject::Connection connection)
    {
        this->connections_.push_back(connection);

        return *this;
    }

    void ConnectionOwner::clear()
    {
        for (auto&& connection : this->connections_)
            QObject::disconnect(connection);

        this->connections_.clear();
    }
}  // namespace ab
