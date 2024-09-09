#pragma once

#include <QColor>
#include <QString>

#include <vector>

namespace chatterino {

[[maybe_unused]] inline const char *const ANONYMOUS_USERNAME = "justinfan64537";

inline constexpr int TWITCH_MESSAGE_LIMIT = 500;

inline QByteArray getDefaultClientID()
{
    return QByteArrayLiteral("7ue61iz46fz11y3cugd0l3tawb4taal");
}

extern const std::vector<QColor> TWITCH_USERNAME_COLORS;

extern const QStringList TWITCH_DEFAULT_COMMANDS;

extern const QStringList TWITCH_WHISPER_COMMANDS;

}  // namespace chatterino
