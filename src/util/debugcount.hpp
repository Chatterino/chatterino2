#pragma once

#include <mutex>
#include <typeinfo>

#include <QMap>
#include <QString>

namespace chatterino {
namespace util {

class DebugCount
{
    static QMap<QString, int64_t> counts;
    static std::mutex mut;

public:
    static void increase(const QString &name)
    {
        std::lock_guard<std::mutex> lock(mut);

        auto it = counts.find(name);
        if (it == counts.end()) {
            counts.insert(name, 1);
        } else {
            reinterpret_cast<int64_t &>(it.value())++;
        }
    }

    static void decrease(const QString &name)
    {
        std::lock_guard<std::mutex> lock(mut);

        auto it = counts.find(name);
        if (it == counts.end()) {
            counts.insert(name, -1);
        } else {
            reinterpret_cast<int64_t &>(it.value())--;
        }
    }

    static QString getDebugText()
    {
        std::lock_guard<std::mutex> lock(mut);

        QString text;
        for (auto it = counts.begin(); it != counts.end(); it++) {
            text += it.key() + ": " + QString::number(it.value()) + "\n";
        }
        return text;
    }

    QString toString()
    {
        return "";
    }
};

}  // namespace util
}  // namespace chatterino
