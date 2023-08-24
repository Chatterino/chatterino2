#pragma once

// This is a helper to define `CHATTERINO_HAS_GLOBAL_SHORTCUT` and forward declare the related classes.

#if __has_include(<QtSystemDetection>)
#    include <QtSystemDetection>
#else
#    include <QtGlobal>
#endif

#if defined(Q_OS_WIN)
// If this is defined, then a backend for global shortcuts exists.
#    define CHATTERINO_HAS_GLOBAL_SHORTCUT
#endif

namespace chatterino {

#ifdef CHATTERINO_HAS_GLOBAL_SHORTCUT

class GlobalShortcutPrivate;

class GlobalShortcut;

#endif

}  // namespace chatterino
