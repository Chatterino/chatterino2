#pragma once

#include <IrcMessage>
#include <QString>

namespace chatterino {

inline QString parseTagString(const QString &input)
{
    QString output = input;
    output.detach();

    auto length = output.length();

    for (int i = 0; i < length - 1; i++)
    {
        if (output[i] == '\\')
        {
            QChar c = output[i + 1];

            switch (c.cell())
            {
                case 'n': {
                    output.replace(i, 2, '\n');
                }
                break;

                case 'r': {
                    output.replace(i, 2, '\r');
                }
                break;

                case 's': {
                    output.replace(i, 2, ' ');
                }
                break;

                case '\\': {
                    output.replace(i, 2, '\\');
                }
                break;

                case ':': {
                    output.replace(i, 2, ';');
                }
                break;

                default: {
                    output.remove(i, 1);
                }
                break;
            }

            i++;
            length--;
        }
    }

    return output;
}

inline QTime calculateMessageTimestamp(const Communi::IrcMessage *message)
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

        return QDateTime::fromMSecsSinceEpoch(ts).time();
    }

    // If present, handle tmi-sent-ts tag and use it as timestamp
    if (message->tags().contains("tmi-sent-ts"))
    {
        auto ts = message->tags().value("tmi-sent-ts").toLongLong();
        return QDateTime::fromMSecsSinceEpoch(ts).time();
    }

    // Some IRC Servers might have server-time tag containing UTC date in ISO format, use it as timestamp
    // See: https://ircv3.net/irc/#server-time
    if (message->tags().contains("time"))
    {
        QString timedate = message->tags().value("time").toString();

        auto date = QDateTime::fromString(timedate, Qt::ISODate);
        date.setTimeSpec(Qt::TimeSpec::UTC);
        return date.toLocalTime().time();
    }

    // Fallback to current time
    return QTime::currentTime();
}

}  // namespace chatterino
