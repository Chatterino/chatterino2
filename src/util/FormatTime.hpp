// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QDateTime>
#include <QString>

#include <chrono>

namespace chatterino {

// format: 1h 23m 42s
// 'components' controls how many most significant components the formatted time will have
QString formatTime(int totalSeconds, int components = 4);
QString formatTime(const QString &totalSecondsString, int components = 4);
QString formatTime(std::chrono::seconds totalSeconds, int components = 4);

/// Formats a duration that's expected to be long (sevreal months or years) like
/// "4 years, 5 days and 8 hours".
///
/// This includes the components year, month, day, and hour.
QString formatLongFriendlyDuration(const QDateTime &from, const QDateTime &to);

}  // namespace chatterino
