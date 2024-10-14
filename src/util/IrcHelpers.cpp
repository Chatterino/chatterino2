#include "util/IrcHelpers.hpp"

#include "Application.hpp"

namespace {

using namespace chatterino;

QDateTime calculateMessageTimeBase(const Communi::IrcMessage *message)
{
    // Check if message is from recent-messages API
    if (message->tags().contains("historical"))
    {
        bool customReceived = false;
        auto ts =
            message->tags().value("rm-received-ts").toLongLong(&customReceived);
        if (!customReceived)
        {
            ts = message->tags().value("tmi-sent-ts").toLongLong();
        }

        return QDateTime::fromMSecsSinceEpoch(ts);
    }

    // If present, handle tmi-sent-ts tag and use it as timestamp
    if (message->tags().contains("tmi-sent-ts"))
    {
        auto ts = message->tags().value("tmi-sent-ts").toLongLong();
        return QDateTime::fromMSecsSinceEpoch(ts);
    }

    // Some IRC Servers might have server-time tag containing UTC date in ISO format, use it as timestamp
    // See: https://ircv3.net/irc/#server-time
    if (message->tags().contains("time"))
    {
        QString timedate = message->tags().value("time").toString();

        auto date = QDateTime::fromString(timedate, Qt::ISODate);
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
