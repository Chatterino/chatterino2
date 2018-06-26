#pragma once

#include "util/helpers.hpp"

#include <QDebug>
#include <QTime>

namespace chatterino {
namespace debug {

template <typename... Args>
inline void Log(const std::string &formatString, Args &&... args)
{
    qDebug().noquote() << QTime::currentTime().toString("hh:mm:ss.zzz")
                       << fS(formatString, std::forward<Args>(args)...).c_str();
}

}  // namespace debug
}  // namespace chatterino
