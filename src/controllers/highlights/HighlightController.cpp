#include "HighlightController.hpp"

#include "Application.hpp"
#include "controllers/highlights/HighlightBlacklistModel.hpp"
#include "controllers/highlights/HighlightModel.hpp"
#include "controllers/highlights/UserHighlightModel.hpp"
#include "widgets/dialogs/NotificationPopup.hpp"

namespace chatterino {

HighlightController::HighlightController()
{
}

template <typename T>
inline void persist(SignalVector<T> &vec, const std::string &name)
{
    auto setting = std::make_unique<ChatterinoSetting<std::vector<T>>>(name);

    for (auto &&item : setting->getValue())
        vec.append(item);

    vec.delayedItemsChanged.connect([setting = setting.get(), vec = &vec] {
        setting->setValue(vec->raw());
    });

    // TODO
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

HighlightModel *HighlightController::createModel(QObject *parent)
{
    HighlightModel *model = new HighlightModel(parent);
    model->init(&this->phrases);

    return model;
}

UserHighlightModel *HighlightController::createUserModel(QObject *parent)
{
    auto *model = new UserHighlightModel(parent);
    model->init(&this->highlightedUsers);

    return model;
}

bool HighlightController::isHighlightedUser(const QString &username)
{
    const auto &userItems = this->highlightedUsers;
    for (const auto &highlightedUser : userItems)
    {
        if (highlightedUser.isMatch(username))
        {
            return true;
        }
    }

    return false;
}

HighlightBlacklistModel *HighlightController::createBlacklistModel(
    QObject *parent)
{
    auto *model = new HighlightBlacklistModel(parent);
    model->init(&this->blacklistedUsers);

    return model;
}

bool HighlightController::blacklistContains(const QString &username)
{
    for (const auto &blacklistedUser : *this->blacklistedUsers.readOnly())
    {
        if (blacklistedUser.isMatch(username))
        {
            return true;
        }
    }

    return false;
}

void HighlightController::addHighlight(const MessagePtr &msg)
{
    //    static NotificationPopup popup;

    //    popup.updatePosition();
    //    popup.addMessage(msg);
    //    popup.show();
}

}  // namespace chatterino
