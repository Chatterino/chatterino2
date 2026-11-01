// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "util/IrcHelpers.hpp"

#include "Application.hpp"

namespace {

using namespace chatterino;

QDateTime calculateMessageTimeBase(const Communi::IrcMessage *message)
{
    // Check if message is from recent-messages API
    auto tags = message->tags();
    if (tags.has("historical"))
    {
        bool customReceived = false;
        auto ts = tags.getOrEmpty("rm-received-ts").toLongLong(&customReceived);
        if (!customReceived)
        {
            ts = tags.getOrEmpty("tmi-sent-ts").toLongLong();
        }

        return QDateTime::fromMSecsSinceEpoch(ts);
    }

    // If present, handle tmi-sent-ts tag and use it as timestamp
    if (auto tmiSentTs = tags.get("tmi-sent-ts"))
    {
        auto ts = tmiSentTs->toLongLong();
        return QDateTime::fromMSecsSinceEpoch(ts);
    }

    // Some IRC Servers might have server-time tag containing UTC date in ISO format, use it as timestamp
    // See: https://ircv3.net/irc/#server-time
    if (auto optTime = message->tags().get("time"))
    {
        auto date = QDateTime::fromString(*optTime, Qt::ISODate);
        date.setTimeZone(QTimeZone::utc());
        return date.toLocalTime();
    }

    // Fallback to current time
#ifdef CHATTERINO_WITH_TESTS
    if (getApp()->isTest())
    {
        return QDateTime::fromMSecsSinceEpoch(0, QTimeZone::utc());
    }
#endif

    return QDateTime::currentDateTime();
}

}  // namespace

namespace chatterino {

QDateTime calculateMessageTime(const Communi::IrcMessage *message)
{
    auto dt = calculateMessageTimeBase(message);

#ifdef CHATTERINO_WITH_TESTS
    if (getApp()->isTest())
    {
        return dt.toUTC();
    }
#endif

    return dt;
}

}  // namespace chatterino
