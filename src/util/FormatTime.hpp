#pragma once

#include <QString>

#include <chrono>

namespace chatterino {

// format: 1h 23m 42s
QString formatTime(int totalSeconds);
QString formatTime(const QString &totalSecondsString);
QString formatTime(std::chrono::seconds totalSeconds);

}  // namespace chatterino
