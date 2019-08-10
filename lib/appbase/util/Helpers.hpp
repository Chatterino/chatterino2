#pragma once

#include <fmt/format.h>
#include <QString>

namespace AB_NAMESPACE {

template <typename... Args>
auto fS(Args &&... args)
{
    return fmt::format(std::forward<Args>(args)...);
}

QString CreateUUID();

QString createLink(const QString &url, bool file = false);

QString createNamedLink(const QString &url, const QString &name,
                        bool file = false);

QString shortenString(const QString &str, unsigned maxWidth = 50);

}  // namespace AB_NAMESPACE

namespace fmt {

// format_arg for QString
inline void format_arg(BasicFormatter<char> &f, const char *&, const QString &v)
{
    f.writer().write("{}", v.toStdString());
}

}  // namespace fmt
