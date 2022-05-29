#include "FormatTime.hpp"

namespace chatterino {

namespace {

    void appendDuration(int count, QChar &&suffix, QString &out)
    {
        if (!out.isEmpty())
        {
            out.append(' ');
        }
        out.append(QString::number(count));
        out.append(suffix);
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
        appendDuration(hours, 'h', res);
    }
    if (minutes > 0)
    {
        appendDuration(minutes, 'm', res);
    }
    if (seconds > 0)
    {
        appendDuration(seconds, 's', res);
    }
    return res;
}

QString formatTime(QString totalSecondsString)
{
    bool ok = true;
    int totalSeconds(totalSecondsString.toInt(&ok));
    if (ok)
    {
        return formatTime(totalSeconds);
    }

    return "n/a";
}

}  // namespace chatterino
