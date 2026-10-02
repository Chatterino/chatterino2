#pragma once

#include <QtCore/QtGlobal>

#if defined(BUILD_CHATTERINO_EMBED_LIB)
#    define CHATTERINO_EMBED_EXPORT Q_DECL_EXPORT
#else
#    define CHATTERINO_EMBED_EXPORT Q_DECL_IMPORT
#endif
