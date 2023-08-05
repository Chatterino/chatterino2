#pragma once

#include <QStringList>

namespace chatterino {

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

enum class XDGDirectory {
    Config,
    Data,
};

QStringList splitEnvironmentVariable(const char *name);
QStringList getXDGDirectories(XDGDirectory directory);

#endif

}  // namespace chatterino
