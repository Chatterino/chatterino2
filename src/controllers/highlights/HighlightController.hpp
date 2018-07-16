#pragma once

#include "common/Singleton.hpp"

#include "common/SignalVector.hpp"
#include "controllers/highlights/HighlightBlacklistUser.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "messages/Message.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

class UserHighlightModel;
class HighlightModel;
class HighlightBlacklistModel;

class HighlightController : public Singleton
{
public:
    HighlightController();

    virtual void initialize(Application &app) override;

    UnsortedSignalVector<HighlightPhrase> phrases;
    UnsortedSignalVector<HighlightBlacklistUser> blacklistedUsers;
    UnsortedSignalVector<HighlightPhrase> highlightedUsers;

    HighlightModel *createModel(QObject *parent);
    HighlightBlacklistModel *createBlacklistModel(QObject *parent);
    UserHighlightModel *createUserModel(QObject *parent);

    bool isHighlightedUser(const QString &username);
    bool blacklistContains(const QString &username);

    void addHighlight(const MessagePtr &msg);

private:
    bool initialized_ = false;

    ChatterinoSetting<std::vector<HighlightPhrase>> highlightsSetting_ = {
        "/highlighting/highlights"};
    ChatterinoSetting<std::vector<HighlightBlacklistUser>> blacklistSetting_ = {
        "/highlighting/blacklist"};
};

}  // namespace chatterino
