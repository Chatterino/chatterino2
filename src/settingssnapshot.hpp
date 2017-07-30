#pragma once

#include "setting.hpp"

namespace chatterino {

struct SettingsSnapshot {
public:
    SettingsSnapshot()
        : _items()
        , _mapItems()
    {
    }

    void addItem(std::reference_wrapper<BaseSetting> setting, const QVariant &value)
    {
        _items.push_back(
            std::pair<std::reference_wrapper<BaseSetting>, QVariant>(setting.get(), value));
    }

    void addMapItem(QString string, QPair<bool, bool> pair)
    {
        QMap<QString, QPair<bool, bool>> map;
        _mapItems.insert(string, pair);
    }

    void apply()
    {
        for (auto &item : _items) {
            item.first.get().setVariant(item.second);
        }
    }

    QMap<QString, QPair<bool, bool>> _mapItems;

private:
    std::vector<std::pair<std::reference_wrapper<BaseSetting>, QVariant>> _items;
};

}  // namespace chatterino
