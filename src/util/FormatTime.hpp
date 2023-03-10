#pragma once

#include <QString>

namespace chatterino {

// format: 1h 23m 42s
QString formatTime(int totalSeconds);
QString formatTime(QString totalSecondsString);

}  // namespace chatterino
