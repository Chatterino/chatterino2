#pragma once

#include "util/UniqueAccess.hpp"

#include <QMap>
#include <QString>

namespace chatterino
{
    class DebugCount
    {
    public:
        static void increase(const QString& name);
        static void decrease(const QString& name);
        static QString getDebugText();

    private:
        static inline UniqueAccess<QMap<QString, int64_t>> counts_{};
    };
}  // namespace chatterino
