#pragma once

#include "common/ChatterinoSetting.hpp"
#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "controllers/highlights/HighlightBlacklistUser.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"

namespace chatterino
{
    struct Message;
    using MessagePtr = std::shared_ptr<const Message>;

    class Settings;
    class Paths;

    class UserHighlightModel;
    class HighlightModel;
    class HighlightBlacklistModel;

    class HighlightController final : public Singleton
    {
    public:
        HighlightController();

        virtual void initialize(Settings& settings, Paths& paths) override;

        UnsortedSignalVector<HighlightPhrase> phrases;
        UnsortedSignalVector<HighlightBlacklistUser> blacklistedUsers;
        UnsortedSignalVector<HighlightPhrase> highlightedUsers;

        HighlightModel* createModel(QObject* parent);
        HighlightBlacklistModel* createBlacklistModel(QObject* parent);
        UserHighlightModel* createUserModel(QObject* parent);

        bool isHighlightedUser(const QString& username);
        bool blacklistContains(const QString& username);

        void addHighlight(const MessagePtr& msg);

    private:
        bool initialized_ = false;

        ChatterinoSetting<std::vector<HighlightPhrase>> highlightsSetting_ = {
            "/highlighting/highlights"};
        ChatterinoSetting<std::vector<HighlightBlacklistUser>>
            blacklistSetting_ = {"/highlighting/blacklist"};
        ChatterinoSetting<std::vector<HighlightPhrase>> userSetting_ = {
            "/highlighting/users"};
    };

}  // namespace chatterino
