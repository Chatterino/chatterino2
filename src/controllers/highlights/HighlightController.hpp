#pragma once

#include "common/SignalVector.hpp"
#include "controllers/highlights/HighlightBlacklistUser.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "controllers/highlights/UserHighlight.hpp"
#include "messages/Message.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

class UserHighlightModel;
class HighlightModel;
class HighlightBlacklistModel;

class HighlightController
{
public:
    HighlightController();

    void initialize();

    UnsortedSignalVector<HighlightPhrase> phrases;
    UnsortedSignalVector<HighlightBlacklistUser> blacklistedUsers;
    UnsortedSignalVector<UserHighlight> highlightedUsers;

    HighlightModel *createModel(QObject *parent);
    HighlightBlacklistModel *createBlacklistModel(QObject *parent);
    UserHighlightModel *createUserModel(QObject *parent);

    bool userContains(const QString &username);
    bool blacklistContains(const QString &username);

    void addHighlight(const MessagePtr &msg);

private:
    bool initialized = false;

    ChatterinoSetting<std::vector<HighlightPhrase>> highlightsSetting = {
        "/highlighting/highlights"};
    ChatterinoSetting<std::vector<HighlightPhrase>> blacklistSetting = {"/highlighting/blacklist"};
};

}  // namespace chatterino
