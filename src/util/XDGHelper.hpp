#pragma once

#include "util/XDGDesktopFile.hpp"

#include <QString>

namespace chatterino {

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

std::optional<XDGDesktopFile> getDefaultBrowserDesktopFile();

/// Parses the given `execKey` and returns the resulting program name, ignoring all arguments
///
/// Parsing is done in accordance to https://specifications.freedesktop.org/desktop-entry-spec/latest/ar01s07.html
///
/// Note: We do *NOT* support field codes
QString parseDesktopExecProgram(const QString &execKey);

#endif

}  // namespace chatterino
