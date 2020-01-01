#include "Helpers.hpp"

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

}  // namespace chatterino
