// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common/Aliases.hpp"
#include "common/Atomic.hpp"
#include "common/UniqueAccess.hpp"
#include "controllers/accounts/Account.hpp"
#include "messages/Emote.hpp"
#include "providers/twitch/TwitchEmotes.hpp"
#include "providers/twitch/TwitchUser.hpp"
#include "util/CancellationToken.hpp"

#include <pajlada/signals.hpp>
#include <QColor>
#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QtContainerFwd>

#include <functional>
#include <memory>
#include <optional>
#include <unordered_set>

namespace chatterino {

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

class TwitchAccount : public Account
{
public:
    TwitchAccount(const QString &username, const QString &oauthToken_,
                  const QString &oauthClient_, const QString &_userID);
    ~TwitchAccount() override;
    TwitchAccount(const TwitchAccount &) = delete;
    TwitchAccount(TwitchAccount &&) = delete;
    TwitchAccount &operator=(const TwitchAccount &) = delete;
    TwitchAccount &operator=(TwitchAccount &&) = delete;

    QString toString() const override;

    const QString &getUserName() const;
    const QString &getOAuthToken() const;
    const QString &getOAuthClient() const;
    const QString &getUserId() const;

    /**
     * The Seventv user-id of the current user. 
     * Empty if there's no associated Seventv user with this twitch user.
     */
    const QString &getSeventvUserID() const;

    QColor color();
    void setColor(QColor color);

    // Attempts to update the users OAuth Client ID
    // Returns true if the value has changed, otherwise false
    bool setOAuthClient(const QString &newClientID);

    // Attempts to update the users OAuth Token
    // Returns true if the value has changed, otherwise false
    bool setOAuthToken(const QString &newOAuthToken);

    // Attempts to update the users username
    // Returns true if the value has changed, otherwise false
    bool setUserName(const QString &newUserName);

    bool isAnon() const;

    void loadBlocks();
    void blockUser(const QString &userId, const QString &userLogin,
                   const QObject *caller, std::function<void()> onSuccess,
                   std::function<void()> onFailure);
    void unblockUser(const QString &userId, const QString &userLogin,
                     const QObject *caller, std::function<void()> onSuccess,
                     std::function<void()> onFailure);

    void blockUserLocally(const QString &userID, const QString &userLogin);

    [[nodiscard]] const std::unordered_set<TwitchUser> &blocks() const;
    [[nodiscard]] const std::unordered_set<QString> &blockedUserIds() const;
    [[nodiscard]] const std::unordered_set<QString> &blockedUserLogins() const;

    // Automod actions
    void autoModAllow(const QString &msgID, ChannelPtr channel) const;
    void autoModDeny(const QString &msgID, ChannelPtr channel) const;

    void loadSeventvUserID();

    /// Returns true if the account has access to the given emote set
    bool hasEmoteSet(const EmoteSetId &id) const;

    /// Returns a map of emote sets the account has access to
    ///
    /// Key being the emote set ID, and contents being information about the emote set
    /// and the emotes contained in the emote set
    SharedAccessGuard<std::shared_ptr<const TwitchEmoteSetMap>>
        accessEmoteSets() const;

    /// Returns a map of emotes the account has access to
    SharedAccessGuard<std::shared_ptr<const EmoteMap>> accessEmotes() const;

    /// Sets the emotes this account has access to
    ///
    /// This should only be used in tests.
    void setEmotes(std::shared_ptr<const EmoteMap> emotes);

    /// Return the emote by emote name if the account has access to the emote
    std::optional<EmotePtr> twitchEmote(const EmoteName &name) const;

    /// Once emotes are reloaded, TwitchAccountManager::emotesReloaded is
    /// invoked with @a caller and an optional error.
    void reloadEmotes(void *caller = nullptr);

private:
    QString oauthClient_;
    QString oauthToken_;
    QString userName_;
    QString userId_;
    const bool isAnon_;
    Atomic<QColor> color_;

    QStringList userstateEmoteSets_;

    ScopedCancellationToken blockToken_;
    std::unordered_set<TwitchUser> ignores_;
    std::unordered_set<QString> ignoresUserIds_;
    std::unordered_set<QString> ignoresUserLogins_;

    ScopedCancellationToken emoteToken_;
    UniqueAccess<std::shared_ptr<const TwitchEmoteSetMap>> emoteSets_;
    UniqueAccess<std::shared_ptr<const EmoteMap>> emotes_;

    QString seventvUserID_;
};

}  // namespace chatterino
