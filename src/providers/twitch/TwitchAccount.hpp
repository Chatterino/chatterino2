#pragma once

#include "common/Aliases.hpp"
#include "common/Atomic.hpp"
#include "common/Channel.hpp"
#include "common/UniqueAccess.hpp"
#include "controllers/accounts/Account.hpp"
#include "messages/Emote.hpp"
#include "providers/twitch/TwitchUser.hpp"
#include "util/QStringHash.hpp"

#include <rapidjson/document.h>
#include <QColor>
#include <QElapsedTimer>
#include <QString>

#include <functional>
#include <mutex>
#include <set>

namespace chatterino {

enum FollowResult {
    FollowResult_Following,
    FollowResult_NotFollowing,
    FollowResult_Failed,
};

struct TwitchEmoteSetResolverResponse {
    const QString channelName;
    const QString channelId;
    const QString type;
    const int tier;
    const bool isCustom;
    // Example response:
    //    {
    //      "channel_name": "zneix",
    //      "channel_id": "99631238",
    //      "type": "",
    //      "tier": 1,
    //      "custom": false
    //    }

    TwitchEmoteSetResolverResponse(QJsonObject jsonObject)
        : channelName(jsonObject.value("channel_name").toString())
        , channelId(jsonObject.value("channel_id").toString())
        , type(jsonObject.value("type").toString())
        , tier(jsonObject.value("tier").toInt())
        , isCustom(jsonObject.value("custom").toBool())
    {
    }
};

std::vector<QStringList> getEmoteSetBatches(QStringList emoteSetKeys);

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
        bool local{false};
        std::vector<TwitchEmote> emotes;
    };

    std::map<QString, EmoteSet> staticEmoteSets;

    struct TwitchAccountEmoteData {
        std::vector<std::shared_ptr<EmoteSet>> emoteSets;

        // this EmoteMap should contain all emotes available globally
        // excluding locally available emotes, such as follower ones
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

    void loadBlocks();
    void blockUser(QString userId, std::function<void()> onSuccess,
                   std::function<void()> onFailure);
    void unblockUser(QString userId, std::function<void()> onSuccess,
                     std::function<void()> onFailure);

    SharedAccessGuard<const std::set<QString>> accessBlockedUserIds() const;
    SharedAccessGuard<const std::set<TwitchUser>> accessBlocks() const;

    void loadEmotes(std::weak_ptr<Channel> weakChannel = {});
    // loadUserstateEmotes loads emote sets that are part of the USERSTATE emote-sets key
    // this function makes sure not to load emote sets that have already been loaded
    void loadUserstateEmotes(std::function<void()> callback);
    // setUserStateEmoteSets sets the emote sets that were parsed from the USERSTATE emote-sets key
    // Returns true if the newly inserted emote sets differ from the ones previously saved
    [[nodiscard]] bool setUserstateEmoteSets(QStringList newEmoteSets);
    SharedAccessGuard<const TwitchAccountEmoteData> accessEmotes() const;
    SharedAccessGuard<const std::unordered_map<QString, EmoteMap>>
        accessLocalEmotes() const;

    // Automod actions
    void autoModAllow(const QString msgID, ChannelPtr channel);
    void autoModDeny(const QString msgID, ChannelPtr channel);

private:
    void loadKrakenEmotes();
    void loadEmoteSetData(std::shared_ptr<EmoteSet> emoteSet);

    QString oauthClient_;
    QString oauthToken_;
    QString userName_;
    QString userId_;
    const bool isAnon_;
    Atomic<QColor> color_;

    mutable std::mutex ignoresMutex_;
    QStringList userstateEmoteSets_;
    UniqueAccess<std::set<TwitchUser>> ignores_;
    UniqueAccess<std::set<QString>> ignoresUserIds_;

    //    std::map<UserId, TwitchAccountEmoteData> emotes;
    UniqueAccess<TwitchAccountEmoteData> emotes_;
    UniqueAccess<std::unordered_map<QString, EmoteMap>> localEmotes_;
};

}  // namespace chatterino
