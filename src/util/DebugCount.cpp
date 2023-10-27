#include "util/DebugCount.hpp"

#include "common/UniqueAccess.hpp"

#include <QLocale>
#include <QStringBuilder>

#include <map>

namespace {

using namespace chatterino;

struct Count {
    int64_t value = 0;
    DebugCount::Flags flags = DebugCount::Flag::None;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
UniqueAccess<std::map<QString, Count>> COUNTS;

}  // namespace

namespace chatterino {

void DebugCount::configure(const QString &name, Flags flags)
{
    auto counts = COUNTS.access();

    auto it = counts->find(name);
    if (it == counts->end())
    {
        counts->emplace(name, Count{.flags = flags});
    }
    else
    {
        it->second.flags = flags;
    }
}

void DebugCount::set(const QString &name, const int64_t &amount)
{
    auto counts = COUNTS.access();

    auto it = counts->find(name);
    if (it == counts->end())
    {
        counts->emplace(name, Count{amount});
    }
    else
    {
        it->second.value = amount;
    }
}

void DebugCount::increase(const QString &name, const int64_t &amount)
{
    auto counts = COUNTS.access();

    auto it = counts->find(name);
    if (it == counts->end())
    {
        counts->emplace(name, Count{amount});
    }
    else
    {
        it->second.value += amount;
    }
}

void DebugCount::decrease(const QString &name, const int64_t &amount)
{
    auto counts = COUNTS.access();

    auto it = counts->find(name);
    if (it == counts->end())
    {
        counts->emplace(name, Count{-amount});
    }
    else
    {
        it->second.value -= amount;
    }
}

QString DebugCount::getDebugText()
{
    auto counts = COUNTS.access();
    QLocale locale;

    QString text;
    for (const auto &[key, count] : *counts)
    {
        text += key % ": " % locale.toString(count.value) % '\n';
    }
    return text;
}

}  // namespace chatterino
