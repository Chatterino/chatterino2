#pragma once

#include <QStringList>

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

namespace chatterino {

enum class XDGDirectory { Config, Data };

QStringList splitEnvironmentVariable(const char *name);
QStringList getXDGDirectories(XDGDirectory directory);

}  // namespace chatterino

#endif
