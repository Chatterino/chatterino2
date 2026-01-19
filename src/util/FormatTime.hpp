// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QDateTime>
#include <QString>

#include <chrono>

namespace chatterino {

// format: 1h 23m 42s
QString formatTime(int totalSeconds);
QString formatTime(const QString &totalSecondsString);
QString formatTime(std::chrono::seconds totalSeconds);

/// Formats a duration that's expected to be long (sevreal months or years) like
/// "4 years, 5 days and 8 hours".
///
/// This includes the components year, month, day, and hour.
QString formatLongFriendlyDuration(const QDateTime &from, const QDateTime &to);

}  // namespace chatterino
