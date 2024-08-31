#pragma once

#include <QStringList>

namespace chatterino {

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

enum class XDGDirectoryType {
    Config,
    Data,
};

/// getXDGDirectories returns a list of directories given a directory type
///
/// This will attempt to read the relevant environment variable (e.g. XDG_CONFIG_HOME and XDG_CONFIG_DIRS) and merge them, with sane defaults
QStringList getXDGDirectories(XDGDirectoryType directory);

#endif

}  // namespace chatterino
