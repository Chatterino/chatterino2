#pragma once

#include "util/helpers.hpp"

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
    detail::_log(fS(formatString, std::forward<Args>(args)...));
}

}  // namespace debug
}  // namespace chatterino
