#pragma once

#include <QMetaObject>
#include <vector>

namespace ab
{
    /// Disconnect a set of Qt event connections on the destructor or the clear
    /// method.
    class ConnectionOwner
    {
    public:
        ConnectionOwner();
        ~ConnectionOwner();

        ConnectionOwner& operator+=(QMetaObject::Connection);
        void clear();

    private:
        std::vector<QMetaObject::Connection> connections_;
    };
}  // namespace ab
