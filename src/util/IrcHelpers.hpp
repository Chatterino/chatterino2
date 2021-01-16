#pragma once

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
        qint64 ts =
            message->tags().value("rm-received-ts").toLongLong(&customReceived);
        if (!customReceived)
        {
            ts = message->tags().value("tmi-sent-ts").toLongLong();
        }

        return QDateTime::fromMSecsSinceEpoch(ts).time();
    }
    else
    {
        return QTime::currentTime();
    }
}

}  // namespace chatterino
