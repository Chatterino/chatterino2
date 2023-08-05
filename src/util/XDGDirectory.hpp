#pragma once

#include <QStringList>

namespace chatterino {

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

enum class XDGDirectoryType {
    Config,
    Data,
};

QStringList getXDGDirectories(XDGDirectoryType directory);

#endif

}  // namespace chatterino
