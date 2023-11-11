#include "providers/twitch/TwitchAccountManager.hpp"

#include "Application.hpp"
#include "common/Args.hpp"
#include "common/Common.hpp"
#include "common/Literals.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/Outcome.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "providers/twitch/TwitchUser.hpp"
#include "util/SharedPtrElementLess.hpp"

namespace chatterino {

using namespace literals;

const QString DEVICE_AUTH_CLIENT_ID = u"ows8k58flcricj1oe1pm53eb78xwql"_s;
const QString DEVICE_AUTH_SCOPES =
    u""_s
    "channel:moderate"  // for seeing automod & which moderator banned/unbanned a user (felanbird unbanned weeb123)
    " channel:read:redemptions"  // for getting the list of channel point redemptions (not currently used)
    " chat:edit"      // for sending messages in chat
    " chat:read"      // for viewing messages in chat
    " whispers:read"  // for viewing recieved whispers

    // https://dev.twitch.tv/docs/api/reference#start-commercial
    " channel:edit:commercial"  // for /commercial api

    // https://dev.twitch.tv/docs/api/reference#create-clip
    " clips:edit"  // for /clip creation

    // https://dev.twitch.tv/docs/api/reference#create-stream-marker
    // https://dev.twitch.tv/docs/api/reference#modify-channel-information
    " channel:manage:broadcast"  // for creating stream markers with /marker command, and for the /settitle and /setgame commands

    // https://dev.twitch.tv/docs/api/reference#get-user-block-list
    " user:read:blocked_users"  // for getting list of blocked users

    // https://dev.twitch.tv/docs/api/reference#block-user
    // https://dev.twitch.tv/docs/api/reference#unblock-user
    " user:manage:blocked_users"  // for blocking/unblocking other users

    // https://dev.twitch.tv/docs/api/reference#manage-held-automod-messages
    " moderator:manage:automod"  // for approving/denying automod messages

    // https://dev.twitch.tv/docs/api/reference#start-a-raid
    // https://dev.twitch.tv/docs/api/reference#cancel-a-raid
    " channel:manage:raids"  // for starting/canceling raids

    // https://dev.twitch.tv/docs/api/reference#create-poll
    // https://dev.twitch.tv/docs/api/reference#end-poll
    " channel:manage:polls"  // for creating & ending polls (not currently used)

    // https://dev.twitch.tv/docs/api/reference#get-polls
    " channel:read:polls"  // for reading broadcaster poll status (not currently used)

    // https://dev.twitch.tv/docs/api/reference#create-prediction
    // https://dev.twitch.tv/docs/api/reference#end-prediction
    " channel:manage:predictions"  // for creating & ending predictions (not currently used)

    // https://dev.twitch.tv/docs/api/reference#get-predictions
    " channel:read:predictions"  // for reading broadcaster prediction status (not currently used)

    // https://dev.twitch.tv/docs/api/reference#send-chat-announcement
    " moderator:manage:announcements"  // for /announce api

    // https://dev.twitch.tv/docs/api/reference#send-whisper
    " user:manage:whispers"  // for whispers api

    // https://dev.twitch.tv/docs/api/reference#ban-user
    // https://dev.twitch.tv/docs/api/reference#unban-user
    " moderator:manage:banned_users"  // for ban/unban/timeout/untimeout api

    // https://dev.twitch.tv/docs/api/reference#delete-chat-messages
    " moderator:manage:chat_messages"  // for delete message api (/delete, /clear)

    // https://dev.twitch.tv/docs/api/reference#update-user-chat-color
    " user:manage:chat_color"  // for update user color api (/color coral)

    // https://dev.twitch.tv/docs/api/reference#get-chat-settings
    " moderator:manage:chat_settings"  // for roomstate api (/followersonly, /uniquechat, /slow)

    // https://dev.twitch.tv/docs/api/reference#get-moderators
    // https://dev.twitch.tv/docs/api/reference#add-channel-moderator
    // https://dev.twitch.tv/docs/api/reference#remove-channel-vip
    " channel:manage:moderators"  // for add/remove/view mod api

    // https://dev.twitch.tv/docs/api/reference#add-channel-vip
    // https://dev.twitch.tv/docs/api/reference#remove-channel-vip
    // https://dev.twitch.tv/docs/api/reference#get-vips
    " channel:manage:vips"  // for add/remove/view vip api

    // https://dev.twitch.tv/docs/api/reference#get-chatters
    " moderator:read:chatters"  // for get chatters api

    // https://dev.twitch.tv/docs/api/reference#get-shield-mode-status
    // https://dev.twitch.tv/docs/api/reference#update-shield-mode-status
    " moderator:manage:shield_mode"  // for reading/managing the channel's shield-mode status

    // https://dev.twitch.tv/docs/api/reference/#send-a-shoutout
    " moderator:manage:shoutouts"  // for reading/managing the channel's shoutouts (not currently used)

    // https://dev.twitch.tv/docs/api/reference/#get-moderated-channels
    " user:read:moderated_channels"  // for reading where the user is modded (not currently used)

    // https://dev.twitch.tv/docs/eventsub/eventsub-subscription-types/#channelchatmessage
    " user:read:chat"  // for reading chat via eventsub (in progress)

    // https://dev.twitch.tv/docs/api/reference/#send-chat-message
    " user:write:chat"  // for sending chat messages via helix (in testing)

    // https://dev.twitch.tv/docs/api/reference/#get-user-emotes
    " user:read:emotes"  // for fetching emotes that a user can use via helix

    // https://dev.twitch.tv/docs/api/reference/#warn-chat-user
    " moderator:manage:warnings"  // for /warn api (and channel.moderate v2 eventsub in the future)

    // https://dev.twitch.tv/docs/api/reference/#get-followed-channels
    " user:read:follows"  // for determining if the current user follows a streamer
    ;

TwitchAccountManager::TwitchAccountManager()
    : accounts(SharedPtrElementLess<TwitchAccount>{})
    , anonymousUser_(new TwitchAccount({.username = ANONYMOUS_USERNAME}))
{
    this->currentUserChanged.connect([this] {
        auto currentUser = this->getCurrent();
        currentUser->loadBlocks();
        currentUser->loadSeventvUserID();
    });

    // We can safely ignore this signal connection since accounts will always be removed
    // before TwitchAccountManager
    std::ignore = this->accounts.itemRemoved.connect([this](const auto &acc) {
        this->removeUser(acc.item.get());
    });

    this->refreshTask_.start(60000);
    QObject::connect(&this->refreshTask_, &QTimer::timeout, [this] {
        this->refreshAccounts(false);
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

    bool listUpdated = false;

    for (const auto &uid : keys)
    {
        if (uid == "current")
        {
            continue;
        }

        auto userData = TwitchAccountData::loadRaw(uid);
        if (!userData)
        {
            continue;
        }

        switch (this->addUser(*userData))
        {
            case AddUserResponse::UserAlreadyExists: {
                qCDebug(chatterinoTwitch)
                    << "User" << userData->username << "already exists";
                // Do nothing
            }
            break;
            case AddUserResponse::UserValuesUpdated: {
                qCDebug(chatterinoTwitch)
                    << "User" << userData->username
                    << "already exists, and values updated!";
                if (userData->username == this->getCurrent()->getUserName())
                {
                    qCDebug(chatterinoTwitch)
                        << "It was the current user, so we need to "
                           "reconnect stuff!";
                    this->currentUserChanged();
                }
            }
            break;
            case AddUserResponse::UserAdded: {
                qCDebug(chatterinoTwitch) << "Added user" << userData->username;
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

        this->refreshAccounts(true);
        this->requestCurrentChecked([this](const auto & /*current*/) {
            this->currentUser_->reloadEmotes();
        });
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

void TwitchAccountManager::refreshAccounts(bool emitChanged)
{
    assertInGuiThread();
    if (this->isRefreshing_)
    {
        return;
    }
    this->isRefreshing_ = true;

    auto launchedRequests = std::make_shared<size_t>(1);
    auto tryFlush = [this, launchedRequests, emitChanged] {
        if (*launchedRequests == 0)
        {
            assert(false && "Called tryFlush after a flush");
            return;
        }

        if (--(*launchedRequests) == 0)
        {
            this->isRefreshing_ = false;
            auto consumers = std::exchange(this->pendingUserConsumers_, {});
            for (const auto &consumer : consumers)
            {
                consumer(this->currentUser_);
            }
            if (emitChanged)
            {
                this->currentUserChanged();
            }
        }
    };

    qCDebug(chatterinoTwitch) << "Checking for accounts to refresh";

    auto current = this->currentUser_;
    auto now = QDateTime::currentDateTimeUtc();
    for (const auto &account : *this->accounts.readOnly())
    {
        if (account->isAnon() ||
            account->type() != TwitchAccount::Type::DeviceAuth)
        {
            continue;
        }
        if (now.secsTo(account->expiresAt()) >= 100)
        {
            continue;
        }
        (*launchedRequests)++;
        qCDebug(chatterinoTwitch)
            << "Refreshing user" << account->getUserName();

        QUrlQuery query{
            {u"client_id"_s, DEVICE_AUTH_CLIENT_ID},
            {u"scope"_s, DEVICE_AUTH_SCOPES},
            {u"refresh_token"_s, account->refreshToken()},
            {u"grant_type"_s, u"refresh_token"_s},
        };
        NetworkRequest("https://id.twitch.tv/oauth2/token",
                       NetworkRequestType::Post)
            .payload(query.toString(QUrl::FullyEncoded).toUtf8())
            .timeout(20000)
            .onSuccess([account, current](const auto &res) {
                const auto json = res.parseJson();
                auto accessToken = json["access_token"_L1].toString();
                auto refreshToken = json["refresh_token"_L1].toString();
                auto expiresIn = json["expires_in"_L1].toInt(-1);
                if (accessToken.isEmpty() || refreshToken.isEmpty() ||
                    expiresIn <= 0)
                {
                    qCWarning(chatterinoTwitch)
                        << "Received invalid OAuth response when refreshing"
                        << account->getUserName();
                    return;
                }
                auto expiresAt =
                    QDateTime::currentDateTimeUtc().addSecs(expiresIn - 120);
                TwitchAccountData data{
                    .username = account->getUserName(),
                    .userID = account->getUserId(),
                    .clientID = DEVICE_AUTH_CLIENT_ID,
                    .oauthToken = accessToken,
                    .ty = TwitchAccount::Type::DeviceAuth,
                    .refreshToken = refreshToken,
                    .expiresAt = expiresAt,
                };
                data.save();
                account->setData(data);
                pajlada::Settings::SettingManager::getInstance()->save();
                qCDebug(chatterinoTwitch)
                    << "Refreshed user" << account->getUserName();

                if (account == current)
                {
                    getHelix()->update(DEVICE_AUTH_CLIENT_ID,
                                       account->getOAuthToken());
                }
            })
            .onError([this, account](const auto &res) {
                auto json = res.parseJson();
                QString message =
                    u"Failed to refresh OAuth token for " %
                    account->getUserName() % u" error: " % res.formatError() %
                    u" - " % json["message"_L1].toString(u"(no message)"_s);
                qCWarning(chatterinoTwitch) << message;

                if (account == this->getCurrent())
                {
                    if (res.status().value_or(0) == 400)  // invalid token
                    {
                        message +=
                            QStringView(u". Consider re-adding your account.");
                    }
                    getApp()->getTwitch()->addGlobalSystemMessage(message);
                }
            })
            .finally(tryFlush)
            .execute();
    }
    tryFlush();  // if no account was refreshed
}

void TwitchAccountManager::requestCurrent(UserCallback cb)
{
    assertInGuiThread();
    if (this->isRefreshing_)
    {
        this->pendingUserConsumers_.emplace_back(std::move(cb));
        return;
    }

    cb(this->currentUser_);
}

void TwitchAccountManager::requestCurrentChecked(UserCallback cb)
{
    assertInGuiThread();
    this->recheckRefresher();
    this->requestCurrent(std::move(cb));
}

void TwitchAccountManager::recheckRefresher()
{
    auto now = QDateTime::currentDateTimeUtc();
    for (const auto &account : *this->accounts.readOnly())
    {
        if (account->isAnon() ||
            account->type() != TwitchAccount::Type::DeviceAuth)
        {
            continue;
        }
        if (now.secsTo(account->expiresAt()) < 1000)
        {
            this->refreshAccounts(false);
            return;
        }
    }
}

TwitchAccountManager::AddUserResponse TwitchAccountManager::addUser(
    const TwitchAccountData &userData)
{
    auto previousUser = this->findUserByUsername(userData.username);
    if (previousUser)
    {
        bool userUpdated = previousUser->setData(userData);

        if (userUpdated)
        {
            return AddUserResponse::UserValuesUpdated;
        }

        return AddUserResponse::UserAlreadyExists;
    }

    auto newUser = std::make_shared<TwitchAccount>(userData);

    //    std::lock_guard<std::mutex> lock(this->mutex);

    this->accounts.insert(newUser);

    return AddUserResponse::UserAdded;
}

}  // namespace chatterino
