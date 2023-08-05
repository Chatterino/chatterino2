#pragma once

#include "util/XDGDesktopFile.hpp"

#include <QString>

namespace chatterino {

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

std::optional<XDGDesktopFile> getDefaultBrowserDesktopFile();

QString parseExeFromDesktopExecKey(QString execKey);

#endif

}  // namespace chatterino
