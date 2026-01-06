// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/twitch/TwitchAccountManager.hpp"

#include "Application.hpp"
#include "common/Args.hpp"
#include "common/Common.hpp"
#include "common/Literals.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "providers/twitch/TwitchUser.hpp"
#include "util/QCompareTransparent.hpp"
#include "util/SharedPtrElementLess.hpp"

#include <QStringBuilder>

namespace {

using namespace chatterino;
using namespace literals;

QString missingScopes(const QJsonArray &scopesArray)
{
    std::set<QString, QCompareTransparent> scopes;
    for (auto s : scopesArray)
    {
        scopes.emplace(s.toString());
    }

    QString missingList;
    for (auto scope : AUTH_SCOPES)
    {
        if (!scopes.contains(scope))
        {
            if (!missingList.isEmpty())
            {
                missingList.append(u", ");
            }
            missingList.append(scope);
        }
    }

    return missingList;
}

void checkMissingScopes(const QString &token)
{
    NetworkRequest(u"https://id.twitch.tv/oauth2/validate"_s,
                   NetworkRequestType::Get)
        .header("Authorization", u"OAuth " % token)
        .timeout(20000)
        .onSuccess([](const auto &res) {
            auto *app = tryGetApp();
            if (!app)
            {
                return;
            }

            const auto json = res.parseJson();
            auto missing = missingScopes(json["scopes"_L1].toArray());
            if (missing.isEmpty())
            {
                return;
            }

            auto msg = MessageBuilder::makeMissingScopesMessage(missing);
            app->getTwitch()->forEachChannel([msg](const auto &chan) {
                chan->addMessage(msg, MessageContext::Original);
            });
        })
        .onError([](const auto &res) {
            qCWarning(chatterinoTwitch)
                << "Failed to check for missing scopes:" << res.formatError();
        })
        .execute();
}

}  // namespace

namespace chatterino {

const std::vector<QStringView> AUTH_SCOPES{
    u"channel:moderate",  // for seeing automod & which moderator banned/unbanned a user (felanbird unbanned weeb123)
    u"channel:read:redemptions",  // for getting the list of channel point redemptions (not currently used)
    u"chat:edit",      // for sending messages in chat
    u"chat:read",      // for viewing messages in chat
    u"whispers:read",  // for viewing recieved whispers

    // https://dev.twitch.tv/docs/api/reference#start-commercial
    u"channel:edit:commercial",  // for /commercial api

    // https://dev.twitch.tv/docs/api/reference#create-clip
    u"clips:edit",  // for /clip creation

    // https://dev.twitch.tv/docs/api/reference#create-stream-marker
    // https://dev.twitch.tv/docs/api/reference#modify-channel-information
    u"channel:manage:broadcast",  // for creating stream markers with /marker command, and for the /settitle and /setgame commands

    // https://dev.twitch.tv/docs/api/reference#get-user-block-list
    u"user:read:blocked_users",  // for getting list of blocked users

    // https://dev.twitch.tv/docs/api/reference#block-user
    // https://dev.twitch.tv/docs/api/reference#unblock-user
    u"user:manage:blocked_users",  // for blocking/unblocking other users

    // https://dev.twitch.tv/docs/api/reference#manage-held-automod-messages
    u"moderator:manage:automod",  // for approving/denying automod messages

    // https://dev.twitch.tv/docs/api/reference#start-a-raid
    // https://dev.twitch.tv/docs/api/reference#cancel-a-raid
    u"channel:manage:raids",  // for starting/canceling raids

    // https://dev.twitch.tv/docs/api/reference#create-poll
    // https://dev.twitch.tv/docs/api/reference#end-poll
    u"channel:manage:polls",  // for creating & ending polls (not currently used)

    // https://dev.twitch.tv/docs/api/reference#get-polls
    u"channel:read:polls",  // for reading broadcaster poll status (not currently used)

    // https://dev.twitch.tv/docs/api/reference#create-prediction
    // https://dev.twitch.tv/docs/api/reference#end-prediction
    u"channel:manage:predictions",  // for creating & ending predictions (not currently used)

    // https://dev.twitch.tv/docs/api/reference#get-predictions
    u"channel:read:predictions",  // for reading broadcaster prediction status (not currently used)

    // https://dev.twitch.tv/docs/api/reference#send-chat-announcement
    u"moderator:manage:announcements",  // for /announce api

    // https://dev.twitch.tv/docs/api/reference#send-whisper
    u"user:manage:whispers",  // for whispers api

    // https://dev.twitch.tv/docs/api/reference#ban-user
    // https://dev.twitch.tv/docs/api/reference#unban-user
    u"moderator:manage:banned_users",  // for ban/unban/timeout/untimeout api & channel.moderate eventsub topic

    // https://dev.twitch.tv/docs/api/reference#delete-chat-messages
    u"moderator:manage:chat_messages",  // for delete message api (/delete, /clear) & channel.moderate eventsub topic

    // https://dev.twitch.tv/docs/api/reference#update-user-chat-color
    u"user:manage:chat_color",  // for update user color api (/color coral)

    // https://dev.twitch.tv/docs/api/reference#get-chat-settings
    u"moderator:manage:chat_settings",  // for roomstate api (/followersonly, /uniquechat, /slow) & channel.moderate eventsub topic

    // https://dev.twitch.tv/docs/api/reference#get-moderators
    // https://dev.twitch.tv/docs/api/reference#add-channel-moderator
    // https://dev.twitch.tv/docs/api/reference#remove-channel-vip
    u"channel:manage:moderators",  // for add/remove/view mod api

    // https://dev.twitch.tv/docs/api/reference#add-channel-vip
    // https://dev.twitch.tv/docs/api/reference#remove-channel-vip
    // https://dev.twitch.tv/docs/api/reference#get-vips
    u"channel:manage:vips",  // for add/remove/view vip api

    // https://dev.twitch.tv/docs/api/reference#get-chatters
    u"moderator:read:chatters",  // for get chatters api

    // https://dev.twitch.tv/docs/api/reference#get-shield-mode-status
    // https://dev.twitch.tv/docs/api/reference#update-shield-mode-status
    u"moderator:manage:shield_mode",  // for reading/managing the channel's shield-mode status

    // https://dev.twitch.tv/docs/api/reference/#send-a-shoutout
    u"moderator:manage:shoutouts",  // for reading/managing the channel's shoutouts (not currently used)

    // https://dev.twitch.tv/docs/api/reference/#get-moderated-channels
    u"user:read:moderated_channels",  // for reading where the user is modded (not currently used)

    // https://dev.twitch.tv/docs/eventsub/eventsub-subscription-types/#channelchatmessage
    u"user:read:chat",  // for reading chat via eventsub (in progress)

    // https://dev.twitch.tv/docs/api/reference/#send-chat-message
    u"user:write:chat",  // for sending chat messages via helix (in testing)

    // https://dev.twitch.tv/docs/api/reference/#get-user-emotes
    u"user:read:emotes",  // for fetching emotes that a user can use via helix

    // https://dev.twitch.tv/docs/api/reference/#warn-chat-user
    u"moderator:manage:warnings",  // for /warn api & channel.moderate eventsub topic

    // https://dev.twitch.tv/docs/api/reference/#get-followed-channels
    u"user:read:follows",  // for determining if the current user follows a streamer

    u"moderator:manage:blocked_terms",  // for channel.moderate eventsub topic

    u"moderator:manage:unban_requests",  // for channel.moderate eventsub topic

    u"moderator:read:moderators",  // for channel.moderate eventsub topic

    u"moderator:read:vips",  // for channel.moderate eventsub topic

    u"moderator:read:suspicious_users",  // for channel.suspicious_user.message and channel.suspicious_user.update
};

TwitchAccountManager::TwitchAccountManager()
    : accounts(SharedPtrElementLess<TwitchAccount>{})
    , anonymousUser_(new TwitchAccount(ANONYMOUS_USERNAME, "", "", ""))
{
    this->currentUserChanged.connect([this] {
        auto currentUser = this->getCurrent();
        currentUser->loadBlocks();
        currentUser->loadSeventvUserID();
        if (!currentUser->isAnon())
        {
            checkMissingScopes(currentUser->getOAuthToken());
        }
    });

    // We can safely ignore this signal connection since accounts will always be removed
    // before TwitchAccountManager
    std::ignore = this->accounts.itemRemoved.connect([this](const auto &acc) {
        this->removeUser(acc.item.get());
    });
}

std::shared_ptr<TwitchAccount> TwitchAccountManager::getCurrent()
{
    if (!this->currentUser_)
    {
        return this->anonymousUser_;
    }

    return this->currentUser_;
}

std::vector<QString> TwitchAccountManager::getUsernames() const
{
    std::vector<QString> userNames;

    std::lock_guard<std::mutex> lock(this->mutex_);

    for (const auto &user : this->accounts)
    {
        userNames.push_back(user->getUserName());
    }

    return userNames;
}

std::shared_ptr<TwitchAccount> TwitchAccountManager::findUserByUsername(
    const QString &username) const
{
    std::lock_guard<std::mutex> lock(this->mutex_);

    for (const auto &user : this->accounts)
    {
        if (username.compare(user->getUserName(), Qt::CaseInsensitive) == 0)
        {
            return user;
        }
    }

    return nullptr;
}

bool TwitchAccountManager::userExists(const QString &username) const
{
    return this->findUserByUsername(username) != nullptr;
}

void TwitchAccountManager::reloadUsers()
{
    auto keys = pajlada::Settings::SettingManager::getObjectKeys("/accounts");

    UserData userData;

    bool listUpdated = false;

    for (const auto &uid : keys)
    {
        if (uid == "current")
        {
            continue;
        }

        auto username = pajlada::Settings::Setting<QString>::get(
            "/accounts/" + uid + "/username");
        auto userID = pajlada::Settings::Setting<QString>::get("/accounts/" +
                                                               uid + "/userID");
        auto clientID = pajlada::Settings::Setting<QString>::get(
            "/accounts/" + uid + "/clientID");
        auto oauthToken = pajlada::Settings::Setting<QString>::get(
            "/accounts/" + uid + "/oauthToken");

        if (username.isEmpty() || userID.isEmpty() || clientID.isEmpty() ||
            oauthToken.isEmpty())
        {
            continue;
        }

        userData.username = username.trimmed();
        userData.userID = userID.trimmed();
        userData.clientID = clientID.trimmed();
        userData.oauthToken = oauthToken.trimmed();

        switch (this->addUser(userData))
        {
            case AddUserResponse::UserAlreadyExists: {
                qCDebug(chatterinoTwitch)
                    << "User" << userData.username << "already exists";
                // Do nothing
            }
            break;
            case AddUserResponse::UserValuesUpdated: {
                qCDebug(chatterinoTwitch)
                    << "User" << userData.username
                    << "already exists, and values updated!";
                if (userData.username == this->getCurrent()->getUserName())
                {
                    qCDebug(chatterinoTwitch)
                        << "It was the current user, so we need to "
                           "reconnect stuff!";
                    this->currentUserChanged();
                }
            }
            break;
            case AddUserResponse::UserAdded: {
                qCDebug(chatterinoTwitch) << "Added user" << userData.username;
                listUpdated = true;
            }
            break;
        }
    }

    if (listUpdated)
    {
        this->userListUpdated.invoke();
    }
}

void TwitchAccountManager::load()
{
    if (getApp()->getArgs().initialLogin.has_value())
    {
        this->currentUsername = getApp()->getArgs().initialLogin.value();
    }

    this->reloadUsers();

    this->currentUsername.connect([this](const QString &newUsername) {
        auto user = this->findUserByUsername(newUsername);

        this->currentUserAboutToChange.invoke(this->currentUser_, user);

        if (user)
        {
            qCDebug(chatterinoTwitch)
                << "Twitch user updated to" << newUsername;
            getHelix()->update(user->getOAuthClient(), user->getOAuthToken());
            this->currentUser_ = user;
        }
        else
        {
            qCDebug(chatterinoTwitch) << "Twitch user updated to anonymous";
            this->currentUser_ = this->anonymousUser_;
        }

        this->currentUserChanged();
        this->currentUser_->reloadEmotes();
    });
}

bool TwitchAccountManager::isLoggedIn() const
{
    if (!this->currentUser_)
    {
        return false;
    }

    // Once `TwitchAccount` class has a way to check, we should also return
    // false if the credentials are incorrect
    return !this->currentUser_->isAnon();
}

bool TwitchAccountManager::removeUser(TwitchAccount *account)
{
    static const QString accountFormat("/accounts/uid%1");

    auto userID(account->getUserId());
    if (!userID.isEmpty())
    {
        pajlada::Settings::SettingManager::removeSetting(
            accountFormat.arg(userID).toStdString());
    }

    if (account->getUserName() == this->currentUsername)
    {
        // The user that was removed is the current user, log into the anonymous
        // user
        this->currentUsername = "";
    }

    this->userListUpdated.invoke();

    return true;
}

TwitchAccountManager::AddUserResponse TwitchAccountManager::addUser(
    const TwitchAccountManager::UserData &userData)
{
    auto previousUser = this->findUserByUsername(userData.username);
    if (previousUser)
    {
        bool userUpdated = false;

        if (previousUser->setOAuthClient(userData.clientID))
        {
            userUpdated = true;
        }

        if (previousUser->setOAuthToken(userData.oauthToken))
        {
            userUpdated = true;
        }

        if (userUpdated)
        {
            return AddUserResponse::UserValuesUpdated;
        }
        else
        {
            return AddUserResponse::UserAlreadyExists;
        }
    }

    auto newUser =
        std::make_shared<TwitchAccount>(userData.username, userData.oauthToken,
                                        userData.clientID, userData.userID);

    //    std::lock_guard<std::mutex> lock(this->mutex);

    this->accounts.insert(newUser);

    return AddUserResponse::UserAdded;
}

}  // namespace chatterino
