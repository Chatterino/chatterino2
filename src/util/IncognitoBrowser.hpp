#pragma once

#include <QtGlobal>

#ifdef Q_OS_WIN

// only supported on windows right now
#    define INCOGNITO_LINKS_SUPPORTED

namespace chatterino {
void openLinkIncognito(const QString &link);
}

#endif
