#pragma once

#include <QHash>
#include <QString>

namespace std {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
template <>
struct hash<QString> {
    std::size_t operator()(const QString &s) const
    {
        return qHash(s);
    }
};
#endif

}  // namespace std
