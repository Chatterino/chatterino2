#include "HighlightController.hpp"

#include "Application.hpp"
#include "common/ChatterinoSetting.hpp"

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

void HighlightController::initialize(Settings &settings, Paths &paths)
{
    assert(!this->initialized_);
    this->initialized_ = true;

    persist(this->phrases, "/highlighting/highlights");
    persist(this->blacklistedUsers, "/highlighting/blacklist");
    persist(this->highlightedUsers, "/highlighting/users");
}

bool HighlightController::isHighlightedUser(const QString &username)
{
    for (const auto &highlightedUser : this->highlightedUsers)
    {
        if (highlightedUser.isMatch(username))
            return true;
    }

    return false;
}

bool HighlightController::blacklistContains(const QString &username)
{
    for (const auto &blacklistedUser : *this->blacklistedUsers.readOnly())
    {
        if (blacklistedUser.isMatch(username))
            return true;
    }

    return false;
}

}  // namespace chatterino
