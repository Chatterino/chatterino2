#pragma once

#include "common/FlagsEnum.hpp"

#include <QString>

namespace chatterino {

class DebugCount
{
public:
    enum class Flag : uint16_t {
        None = 0,
        /// The value is a data size in bytes
        DataSize = 1 << 0,
    };
    using Flags = FlagsEnum<Flag>;

    static void configure(const QString &name, Flags flags);

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
