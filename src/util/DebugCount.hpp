#pragma once

#include <common/UniqueAccess.hpp>

#include <mutex>
#include <typeinfo>

#include <QMap>
#include <QString>

namespace chatterino
{
    class DebugCount
    {
    public:
        static void increase(const QString& name)
        {
            auto counts = counts_.access();

            auto it = counts->find(name);
            if (it == counts->end())
            {
                counts->insert(name, 1);
            }
            else
            {
                reinterpret_cast<int64_t&>(it.value())++;
            }
        }

        static void decrease(const QString& name)
        {
            auto counts = counts_.access();

            auto it = counts->find(name);
            if (it == counts->end())
            {
                counts->insert(name, -1);
            }
            else
            {
                reinterpret_cast<int64_t&>(it.value())--;
            }
        }

        static QString getDebugText()
        {
            auto counts = counts_.access();

            QString text;
            for (auto it = counts->begin(); it != counts->end(); it++)
            {
                text += it.key() + ": " + QString::number(it.value()) + "\n";
            }
            return text;
        }

        QString toString()
        {
            return "";
        }

    private:
        static UniqueAccess<QMap<QString, int64_t>> counts_;
    };
}  // namespace chatterino
