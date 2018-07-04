#pragma once

#include "common/SignalVector.hpp"
#include "controllers/highlights/HighlightBlacklistUser.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "messages/Message.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

class HighlightModel;
class HighlightBlacklistModel;

class HighlightController
{
public:
    HighlightController();

    void initialize();

    UnsortedSignalVector<HighlightPhrase> phrases;
    UnsortedSignalVector<HighlightBlacklistUser> blacklistedUsers;

    HighlightModel *createModel(QObject *parent);
    HighlightBlacklistModel *createBlacklistModel(QObject *parent);

    bool blacklistContains(const QString &username);

    void addHighlight(const MessagePtr &msg);

private:
    bool initialized = false;

    ChatterinoSetting<std::vector<HighlightPhrase>> highlightsSetting = {
        "/highlighting/highlights"};
    ChatterinoSetting<std::vector<HighlightPhrase>> blacklistSetting = {"/highlighting/blacklist"};
};

}  // namespace chatterino
