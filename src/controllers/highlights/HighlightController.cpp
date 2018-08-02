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

    for (const HighlightPhrase &phrase : this->highlightsSetting_.getValue()) {
        this->phrases.appendItem(phrase);
    }

    this->phrases.delayedItemsChanged.connect([this] {  //
        this->highlightsSetting_.setValue(this->phrases.getVector());
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
    const auto &userItems = this->highlightedUsers.getVector();
    for (const auto &highlightedUser : userItems) {
        if (highlightedUser.isMatch(username)) {
            return true;
        }
    }

    return false;
}

HighlightBlacklistModel *HighlightController::createBlacklistModel(QObject *parent)
{
    auto *model = new HighlightBlacklistModel(parent);
    model->init(&this->blacklistedUsers);

    return model;
}

bool HighlightController::blacklistContains(const QString &username)
{
    std::vector<HighlightBlacklistUser> blacklistItems = this->blacklistedUsers.getVector();
    for (const auto &blacklistedUser : blacklistItems) {
        if (blacklistedUser.isMatch(username)) {
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
