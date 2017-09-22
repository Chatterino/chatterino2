#pragma once

#include "setting.hpp"

namespace chatterino {

struct SettingsSnapshot {
public:
    SettingsSnapshot()
    {
    }

    void addItem(std::reference_wrapper<BaseSetting> setting, const QVariant &value)
    {
        this->items.push_back(
            std::pair<std::reference_wrapper<BaseSetting>, QVariant>(setting.get(), value));
    }

    void addMapItem(QString string, QPair<bool, bool> pair)
    {
        QMap<QString, QPair<bool, bool>> map;
        this->mapItems.insert(string, pair);
    }

    void apply()
    {
        for (auto &item : this->items) {
            item.first.get().setVariant(item.second);
        }
    }

    QMap<QString, QPair<bool, bool>> mapItems;

private:
    std::vector<std::pair<std::reference_wrapper<BaseSetting>, QVariant>> items;
};

}  // namespace chatterino
