#pragma once

#include <memory>
#include "common/ChatterinoSetting.hpp"
#include "common/SignalVector.hpp"

namespace chatterino {

template <typename T>
inline void persist(SignalVector<T> &vec, const std::string &name)
{
    auto setting = std::make_unique<ChatterinoSetting<std::vector<T>>>(name);

    for (auto &&item : setting->getValue())
        vec.append(item);

    vec.delayedItemsChanged.connect([setting = setting.get(), vec = &vec] {
        setting->setValue(vec->raw());
    });

    // TODO: Delete when appropriate.
    setting.release();
}

}  // namespace chatterino
