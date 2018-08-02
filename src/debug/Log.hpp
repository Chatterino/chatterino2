#pragma once

#include "util/Helpers.hpp"

#include <QDebug>
#include <QTime>

namespace chatterino {

template <typename... Args>
inline void Log(const std::string &formatString, Args &&... args)
{
    qDebug().noquote() << QTime::currentTime().toString("hh:mm:ss.zzz")
                       << fS(formatString, std::forward<Args>(args)...).c_str();
}

template <typename... Args>
inline void Warn(const std::string &formatString, Args &&... args)
{
    qWarning() << QTime::currentTime().toString("hh:mm:ss.zzz")
               << fS(formatString, std::forward<Args>(args)...).c_str();
}

}  // namespace chatterino
