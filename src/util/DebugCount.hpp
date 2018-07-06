#pragma once

#include <mutex>
#include <typeinfo>

#include <QMap>
#include <QString>

namespace chatterino {

class DebugCount
{
public:
    static void increase(const QString &name)
    {
        std::lock_guard<std::mutex> lock(mut_);

        auto it = counts_.find(name);
        if (it == counts_.end()) {
            counts_.insert(name, 1);
        } else {
            reinterpret_cast<int64_t &>(it.value())++;
        }
    }

    static void decrease(const QString &name)
    {
        std::lock_guard<std::mutex> lock(mut_);

        auto it = counts_.find(name);
        if (it == counts_.end()) {
            counts_.insert(name, -1);
        } else {
            reinterpret_cast<int64_t &>(it.value())--;
        }
    }

    static QString getDebugText()
    {
        std::lock_guard<std::mutex> lock(mut_);

        QString text;
        for (auto it = counts_.begin(); it != counts_.end(); it++) {
            text += it.key() + ": " + QString::number(it.value()) + "\n";
        }
        return text;
    }

    QString toString()
    {
        return "";
    }

private:
    static QMap<QString, int64_t> counts_;
    static std::mutex mut_;
};

}  // namespace chatterino
