#pragma once

#include "common/Aliases.hpp"
#include "common/Atomic.hpp"
#include "common/UniqueAccess.hpp"
#include "controllers/accounts/Account.hpp"
#include "messages/Emote.hpp"
#include "providers/twitch/TwitchUser.hpp"

#include <rapidjson/document.h>
#include <QColor>
#include <QString>

#include <functional>
#include <mutex>
#include <set>

namespace chatterino {

enum IgnoreResult {
    IgnoreResult_Success,
    IgnoreResult_AlreadyIgnored,
    IgnoreResult_Failed,
};

enum UnignoreResult {
    UnignoreResult_Success,
    UnignoreResult_Failed,
};

enum FollowResult {
    FollowResult_Following,
    FollowResult_NotFollowing,
    FollowResult_Failed,
};

class TwitchAccount : public Account
{
public:
    struct TwitchEmote {
        EmoteId id;
        EmoteName name;
    };

    struct EmoteSet {
        QString key;
        QString channelName;
        QString text;
        QString type;
        std::vector<TwitchEmote> emotes;
    };

    std::map<QString, EmoteSet> staticEmoteSets;

    struct TwitchAccountEmoteData {
        std::vector<std::shared_ptr<EmoteSet>> emoteSets;

        std::vector<EmoteName> allEmoteNames;

        EmoteMap emotes;
    };

    TwitchAccount(const QString &username, const QString &oauthToken_,
                  const QString &oauthClient_, const QString &_userID);

    virtual QString toString() const override;

    const QString &getUserName() const;
    const QString &getOAuthToken() const;
    const QString &getOAuthClient() const;
    const QString &getUserId() const;

    QColor color();
    void setColor(QColor color);

    // Attempts to update the users OAuth Client ID
    // Returns true if the value has changed, otherwise false
    bool setOAuthClient(const QString &newClientID);

    // Attempts to update the users OAuth Token
    // Returns true if the value has changed, otherwise false
    bool setOAuthToken(const QString &newOAuthToken);

    bool isAnon() const;

    void loadIgnores();
    void ignore(const QString &targetName,
                std::function<void(IgnoreResult, const QString &)> onFinished);
    void ignoreByID(
        const QString &targetUserID, const QString &targetName,
        std::function<void(IgnoreResult, const QString &)> onFinished);
    void unignore(
        const QString &targetName,
        std::function<void(UnignoreResult, const QString &)> onFinished);
    void unignoreByID(
        const QString &targetUserID, const QString &targetName,
        std::function<void(UnignoreResult, const QString &message)> onFinished);

    void checkFollow(const QString targetUserID,
                     std::function<void(FollowResult)> onFinished);
    void followUser(const QString userID,
                    std::function<void()> successCallback);
    void unfollowUser(const QString userID,
                      std::function<void()> successCallback);

    std::set<TwitchUser> getIgnores() const;

    void loadEmotes();
    AccessGuard<const TwitchAccountEmoteData> accessEmotes() const;

    // Automod actions
    void autoModAllow(const QString msgID,
                      std::function<void()> successCallback);
    void autoModDeny(const QString msgID,
                     std::function<void()> successCallback);

private:
    void parseEmotes(const rapidjson::Document &document);
    void loadEmoteSetData(std::shared_ptr<EmoteSet> emoteSet);

    QString oauthClient_;
    QString oauthToken_;
    QString userName_;
    QString userId_;
    const bool isAnon_;
    Atomic<QColor> color_;

    mutable std::mutex ignoresMutex_;
    std::set<TwitchUser> ignores_;

    //    std::map<UserId, TwitchAccountEmoteData> emotes;
    UniqueAccess<TwitchAccountEmoteData> emotes_;
};

}  // namespace chatterino
