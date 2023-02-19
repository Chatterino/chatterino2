#pragma once

#include "util/XDGDesktopFile.hpp"

#include <QString>

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

namespace chatterino {

std::optional<XDGDesktopFile> getDefaultBrowserDesktopFile();

QString parseExeFromDesktopExecKey(QString execKey);

}  // namespace chatterino

#endif
