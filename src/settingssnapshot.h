#pragma once

#include "setting.h"

namespace chatterino {

struct SettingsSnapshot {
public:
    SettingsSnapshot()
        : _items()
    {
    }

    void addItem(std::reference_wrapper<BaseSetting> setting, const QVariant &value)
    {
        _items.push_back(
            std::pair<std::reference_wrapper<BaseSetting>, QVariant>(setting.get(), value));
    }

    void apply()
    {
        for (auto &item : _items) {
            item.first.get().setVariant(item.second);
        }
    }

private:
    std::vector<std::pair<std::reference_wrapper<BaseSetting>, QVariant>> _items;
};

}  // namespace chatterino
