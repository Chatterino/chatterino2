#pragma once

#include <QDir>
#include <QString>

namespace chatterino {
namespace util {

// https://stackoverflow.com/a/13014491
static QString combinePath(const QString &a, const QString &b)
{
    return QDir::cleanPath(a + QDir::separator() + b);
}

}  // namespace util
}  // namespace chatterino
