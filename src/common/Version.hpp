#pragma once

#include <QtGlobal>

#define CHATTERINO_VERSION "2.1.4-beta-2"

#if defined(Q_OS_WIN)
#    define CHATTERINO_OS "win"
#elif defined(Q_OS_MACOS)
#    define CHATTERINO_OS "macos"
#elif defined(Q_OS_LINUX)
#    define CHATTERINO_OS "linux"
#else
#    define CHATTERINO_OS "unknown"
#endif
