#pragma once

#include <QDir>
#include <QString>

namespace chatterino {

// https://stackoverflow.com/a/13014491
inline QString combinePath(const QString &a, const QString &b)
{
    return QDir::cleanPath(a + QDir::separator() + b);
}

}  // namespace chatterino
