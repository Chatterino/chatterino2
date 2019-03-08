#include "DebugCount.hpp"

namespace chatterino
{
    void DebugCount::increase(const QString& name)
    {
        auto counts = counts_.access();

        if (auto it = counts->find(name); it != counts->end())
            reinterpret_cast<int64_t&>(it.value())++;
        else
            counts->insert(name, 1);
    }

    void DebugCount::decrease(const QString& name)
    {
        auto counts = counts_.access();

        if (auto it = counts->find(name); it != counts->end())
            reinterpret_cast<int64_t&>(it.value())--;
        else
            counts->insert(name, -1);
    }

    QString DebugCount::getDebugText()
    {
        auto counts = counts_.access();

        QString text;

        // manual iteration required to get key AND value
        for (auto it = counts->begin(); it != counts->end(); it++)
        {
            text += it.key() + ": " + QString::number(it.value()) + "\n";
        }
        return text;
    }
}  // namespace chatterino
