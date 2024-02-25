#pragma once

#include <boost/container_hash/hash_fwd.hpp>
#include <QHash>
#include <QString>

#include <functional>

namespace boost {

template <>
struct hash<QString> {
    std::size_t operator()(QString const &s) const
    {
        return qHash(s);
    }
};

}  // namespace boost

namespace std {

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
template <>
struct hash<QString> {
    std::size_t operator()(const QString &s) const
    {
        return qHash(s);
    }
};
#endif

}  // namespace std
