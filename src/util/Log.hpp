#pragma once

#include "util/Helpers.hpp"

#include <QDebug>
#include <QTime>

namespace chatterino
{
    template <typename... Args>
    inline void log(const std::string& formatString, Args&&... args)
    {
        qDebug().noquote()
            << QTime::currentTime().toString("hh:mm:ss.zzz")
            << fS(formatString, std::forward<Args>(args)...).c_str();
    }

    template <typename... Args>
    inline void log(const char* formatString, Args&&... args)
    {
        log(std::string(formatString), std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void log(const QString& formatString, Args&&... args)
    {
        log(formatString.toStdString(), std::forward<Args>(args)...);
    }
}  // namespace chatterino
