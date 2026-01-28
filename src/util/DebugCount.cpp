// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "util/DebugCount.hpp"

#include "common/UniqueAccess.hpp"
#include "util/QMagicEnum.hpp"

#include <magic_enum/magic_enum.hpp>
#include <QLocale>
#include <QStringBuilder>

#include <array>

namespace {

using namespace chatterino;

struct Count {
    int64_t value = 0;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
UniqueAccess<std::array<Count, static_cast<size_t>(DebugObject::Count)>> COUNTS;

constexpr bool isBytes(DebugObject target)
{
    switch (target)
    {
        default:
            return false;

        case DebugObject::BytesImageCurrent:
        case DebugObject::BytesImageLoaded:
        case DebugObject::BytesImageUnloaded:
            return true;
    }
}

}  // namespace

namespace chatterino {

void DebugCount::set(DebugObject target, int64_t amount)
{
    auto counts = COUNTS.access();

    auto &it = counts->at(static_cast<size_t>(target));
    it.value = amount;
}

void DebugCount::increase(DebugObject target, int64_t amount)
{
    auto counts = COUNTS.access();

    auto &it = counts->at(static_cast<size_t>(target));
    it.value += amount;
}

void DebugCount::decrease(DebugObject target, int64_t amount)
{
    auto counts = COUNTS.access();

    auto &it = counts->at(static_cast<size_t>(target));
    it.value -= amount;
}

QString DebugCount::getDebugText()
{
    static const QLocale locale(QLocale::English);

    auto counts = COUNTS.access();

    QString text;
    for (size_t key = 0; key < static_cast<size_t>(DebugObject::Count); key++)
    {
        auto &count = counts->at(key);

        QString formatted;
        if (isBytes(static_cast<DebugObject>(key)))
        {
            formatted = locale.formattedDataSize(count.value);
        }
        else
        {
            formatted = locale.toString(static_cast<qlonglong>(count.value));
        }

        text += qmagicenum::enumName(static_cast<DebugObject>(key)) % ": " %
                formatted % '\n';
    }
    return text;
}

}  // namespace chatterino
