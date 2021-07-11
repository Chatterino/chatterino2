#include "Helpers.hpp"

#include "providers/twitch/TwitchCommon.hpp"

#include <QLocale>
#include <QUuid>

namespace chatterino {

QString generateUuid()
{
    auto uuid = QUuid::createUuid();
    return uuid.toString();
}

QString formatRichLink(const QString &url, bool file)
{
    return QString("<a href=\"") + (file ? "file:///" : "") + url + "\">" +
           url + "</a>";
}

QString formatRichNamedLink(const QString &url, const QString &name, bool file)
{
    return QString("<a href=\"") + (file ? "file:///" : "") + url + "\">" +
           name + "</a>";
}

QString shortenString(const QString &str, unsigned maxWidth)
{
    auto shortened = QString(str);

    if (str.size() > int(maxWidth))
    {
        shortened.resize(int(maxWidth));
        shortened += "...";
    }

    return shortened;
}

QString localizeNumbers(const int &number)
{
    QLocale locale;
    return locale.toString(number);
}

QString kFormatNumbers(const int &number)
{
    return QString("%1K").arg(number / 1000);
}

QColor getRandomColor(const QString &userId)
{
    bool ok = true;
    int colorSeed = userId.toInt(&ok);
    if (!ok)
    {
        // We were unable to convert the user ID to an integer, this means Twitch started to use non-integer user IDs (or we're on IRC)
        // Use sum of unicode values of all characters in id / IRC nick
        colorSeed = 0;
        for (const auto &c : userId)
        {
            colorSeed += c.digitValue();
        }
    }

    const auto colorIndex = colorSeed % TWITCH_USERNAME_COLORS.size();
    return TWITCH_USERNAME_COLORS[colorIndex];
}

}  // namespace chatterino
