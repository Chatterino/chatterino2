#ifndef SETTINGSSNAPSHOT_H
#define SETTINGSSNAPSHOT_H

#include "setting.h"

struct SettingsSnapshot {
private:
    std::vector<
        std::pair<std::reference_wrapper<chatterino::BaseSetting>, QVariant>>
        items;

public:
    SettingsSnapshot()
        : items()
    {
    }

    void
    addItem(std::reference_wrapper<chatterino::BaseSetting> setting,
            const QVariant &value)
    {
        items.push_back(
            std::pair<std::reference_wrapper<chatterino::BaseSetting>,
                      QVariant>(setting.get(), value));
    }

    void
    apply()
    {
        for (auto &item : this->items) {
            item.first.get().setVariant(item.second);
        }
    }
};

#endif  // SETTINGSSNAPSHOT_H
