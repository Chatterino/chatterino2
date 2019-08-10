#include "Helpers.hpp"

#include <QUuid>

namespace AB_NAMESPACE {

QString CreateUUID()
{
    auto uuid = QUuid::createUuid();
    return uuid.toString();
}

QString createLink(const QString &url, bool file)
{
    return QString("<a href=\"") + (file ? "file:///" : "") + url + "\">" +
           url + "</a>";
}

QString createNamedLink(const QString &url, const QString &name, bool file)
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

}  // namespace AB_NAMESPACE
