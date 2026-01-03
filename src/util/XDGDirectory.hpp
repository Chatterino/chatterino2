#pragma once

#include <QStringList>

#include <cstdint>

namespace chatterino {

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

enum class XDGDirectoryType : std::uint8_t {
    /// The values from XDG_CONFIG_HOME and XDG_CONFIG_DIRS.
    Config,
    /// The values from XDG_DATA_HOME and XDG_DATA_DIRS
    Data,
};

/// Return the directories from the base environment variables only (e.g. XDG_CONFIG_DIRS)
QStringList getXDGBaseDirectories(XDGDirectoryType directory);

/// Return the directories from the user environment variables only (e.g. XDG_CONFIG_HOME)
QStringList getXDGUserDirectories(XDGDirectoryType directory);

/// getXDGDirectories returns a list of directories given a directory type
///
/// This will attempt to read the relevant environment variable (e.g. XDG_CONFIG_HOME and XDG_CONFIG_DIRS) and merge them, with sane defaults
QStringList getXDGDirectories(XDGDirectoryType directory);

#endif

}  // namespace chatterino
