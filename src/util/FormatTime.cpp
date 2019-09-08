#include "FormatTime.hpp"

namespace chatterino {
namespace {
    void appendDuration(int count, QChar &&order, QString &outString)
    {
        outString.append(QString::number(count));
        outString.append(order);
    }
}  // namespace

QString formatTime(int totalSeconds)
{
    QString res;

    int seconds = totalSeconds % 60;
    int timeoutMinutes = totalSeconds / 60;
    int minutes = timeoutMinutes % 60;
    int timeoutHours = timeoutMinutes / 60;
    int hours = timeoutHours % 24;
    int days = timeoutHours / 24;
    if (days > 0)
    {
        appendDuration(days, 'd', res);
    }
    if (hours > 0)
    {
        if (!res.isEmpty())
        {
            res.append(" ");
        }
        appendDuration(hours, 'h', res);
    }
    if (minutes > 0)
    {
        if (!res.isEmpty())
        {
            res.append(" ");
        }
        appendDuration(minutes, 'm', res);
    }
    if (seconds > 0)
    {
        if (!res.isEmpty())
        {
            res.append(" ");
        }
        appendDuration(seconds, 's', res);
    }
    return res;
}

}  // namespace chatterino
