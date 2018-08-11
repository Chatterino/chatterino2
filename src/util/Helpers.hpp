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

}  // namespace chatterino

namespace fmt {

// format_arg for QString
inline void format_arg(BasicFormatter<char> &f, const char *&, const QString &v)
{
    f.writer().write("{}", v.toStdString());
}

}  // namespace fmt
