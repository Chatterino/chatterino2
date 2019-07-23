#pragma once

#include <fmt/format.h>
#include <QUuid>

namespace AB_NAMESPACE {

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
    return QString("<a href=\"") + (file ? "file:///" : "") + url + "\">" +
           url + "</a>";
}

static QString createNamedLink(const QString &url, const QString &name,
                               bool file = false)
{
    return QString("<a href=\"") + (file ? "file:///" : "") + url + "\">" +
           name + "</a>";
}

static QString shortenString(const QString &str, unsigned maxWidth = 50)
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

namespace fmt {

// format_arg for QString
inline void format_arg(BasicFormatter<char> &f, const char *&, const QString &v)
{
    f.writer().write("{}", v.toStdString());
}

}  // namespace fmt
