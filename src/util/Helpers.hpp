#pragma once

#include <fmt/format.h>
#include <QUuid>

namespace chatterino {

template <typename... Args>
auto fS(Args &&... args)
{
    return fmt::format(std::forward<Args>(args)...);
}

static QString CreateUUID()
{
    auto uuid = QUuid::createUuid();
    return uuid.toString();
}

static QString createLink(const QString &url, bool file = false)
{
    if (file) {
        return QString("<a href=\"file:///" + url +
                       "\"><span style=\"color: white;\">" + url +
                       "</span></a>");
    }

    return QString("<a href=\"" + url + "\"><span style=\"color: white;\">" +
                   url + "</span></a>");
}

static QString createNamedLink(const QString &url, const QString &name, bool file = false)
{
    if (file) {
        return QString("<a href=\"file:///" + url +
                       "\"><span style=\"color: white;\">" + name +
                       "</span></a>");
    }

    return QString("<a href=\"" + url + "\"><span style=\"color: white;\">" +
                   name + "</span></a>");
}

static QString shortenString(const QString &str, unsigned maxWidth = 50)
{
    if (str.size() <= maxWidth) {
        return str;
    }

    QString shortenedStr = str;
    shortenedStr.resize(47);
    shortenedStr += "...";

    return shortenedStr;
}

}  // namespace chatterino

namespace fmt {

// format_arg for QString
inline void format_arg(BasicFormatter<char> &f, const char *&, const QString &v)
{
    f.writer().write("{}", v.toStdString());
}

}  // namespace fmt
