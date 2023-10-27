#pragma once

#include <QString>

namespace chatterino {

class DebugCount
{
public:
    static void set(const QString &name, const int64_t &amount);

    static void increase(const QString &name, const int64_t &amount);
    static void increase(const QString &name)
    {
        DebugCount::increase(name, 1);
    }

    static void decrease(const QString &name, const int64_t &amount);
    static void decrease(const QString &name)
    {
        DebugCount::decrease(name, 1);
    }

    static QString getDebugText();
};

}  // namespace chatterino
