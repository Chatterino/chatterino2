#pragma once

#include <fmt/format.h>

namespace chatterino {

template <typename... Args>
auto fS(Args &&... args) -> decltype(fmt::format(std::forward<Args>(args)...))
{
    return fmt::format(std::forward<Args>(args)...);
}

}  // namespace chatterino

namespace fmt {

// format_arg for QString
inline void format_arg(BasicFormatter<char> &f, const char *&, const QString &v)
{
    f.writer().write("\"{}\"", v.toStdString());
}

}  // namespace fmt
