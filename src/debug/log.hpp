#pragma once

#include "fmt/format.h"

#include <QDebug>
#include <QTime>

namespace chatterino {
namespace debug {

namespace detail {

static void _log(const std::string &message)
{
    qDebug().noquote() << QTime::currentTime().toString("hh:mm:ss.zzz") << message.c_str();
}

}  // namespace detail

template <typename... Args>
inline void Log(const std::string &formatString, Args &&... args)
{
    detail::_log(fmt::format(formatString, std::forward<Args>(args)...));
}

}  // namespace debug
}  // namespace chatterino

namespace fmt {

// format_arg for QString
inline void format_arg(BasicFormatter<char> &f, const char *&, const QString &v)
{
    f.writer().write("\"{}\"", v.toStdString());
}

}  // namespace fmt
