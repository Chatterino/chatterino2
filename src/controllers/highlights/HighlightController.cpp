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

void HighlightController::initialize(Settings &settings, Paths &paths)
{
    assert(!this->initialized_);
    this->initialized_ = true;

    for (const HighlightPhrase &phrase : this->highlightsSetting_.getValue())
    {
        this->phrases.appendItem(phrase);
    }

    this->phrases.delayedItemsChanged.connect([this] {  //
        this->highlightsSetting_.setValue(this->phrases.getVector());
    });

    for (const HighlightBlacklistUser &blacklistedUser :
         this->blacklistSetting_.getValue())
    {
        this->blacklistedUsers.appendItem(blacklistedUser);
    }

    this->blacklistedUsers.delayedItemsChanged.connect([this] {
        this->blacklistSetting_.setValue(this->blacklistedUsers.getVector());
    });

    for (const HighlightPhrase &user : this->userSetting_.getValue())
    {
        this->highlightedUsers.appendItem(user);
    }

    this->highlightedUsers.delayedItemsChanged.connect([this] {  //
        this->userSetting_.setValue(this->highlightedUsers.getVector());
    });
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
    for (const auto &blacklistedUser : this->blacklistedUsers)
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
