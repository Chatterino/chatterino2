#include "util/DebugCount.hpp"

#include "common/UniqueAccess.hpp"

#include <QMap>

namespace {

using namespace chatterino;

struct Count {
    int64_t value = 0;
    DebugCount::Flags flags = DebugCount::Flag::None;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
UniqueAccess<QMap<QString, Count>> COUNTS;

}  // namespace

namespace chatterino {

void DebugCount::configure(const QString &name, Flags flags)
{
    auto counts = COUNTS.access();

    auto it = counts->find(name);
    if (it == counts->end())
    {
        counts->insert(name, {.flags = flags});
    }
    else
    {
        it.value().flags = flags;
    }
}

void DebugCount::set(const QString &name, const int64_t &amount)
{
    auto counts = COUNTS.access();

    auto it = counts->find(name);
    if (it == counts->end())
    {
        counts->insert(name, {amount});
    }
    else
    {
        it.value().value = amount;
    }
}

void DebugCount::increase(const QString &name, const int64_t &amount)
{
    auto counts = COUNTS.access();

    auto it = counts->find(name);
    if (it == counts->end())
    {
        counts->insert(name, {amount});
    }
    else
    {
        it.value().value += amount;
    }
}

void DebugCount::decrease(const QString &name, const int64_t &amount)
{
    auto counts = COUNTS.access();

    auto it = counts->find(name);
    if (it == counts->end())
    {
        counts->insert(name, {-amount});
    }
    else
    {
        it.value().value -= amount;
    }
}

QString DebugCount::getDebugText()
{
    auto counts = COUNTS.access();

    QString text;
    for (auto it = counts->begin(); it != counts->end(); it++)
    {
        text += it.key() + ": " + QString::number(it.value().value) + "\n";
    }
    return text;
}

}  // namespace chatterino
