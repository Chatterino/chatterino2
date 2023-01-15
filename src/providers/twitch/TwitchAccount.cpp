#include "providers/twitch/TwitchAccount.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/irc/IrcMessageBuilder.hpp"
#include "providers/IvrApi.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchUser.hpp"
#include "singletons/Emotes.hpp"
#include "util/Helpers.hpp"
#include "util/QStringHash.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <QThread>

namespace chatterino {

TwitchAccount::TwitchAccount(const QString &username, const QString &oauthToken,
                             const QString &oauthClient, const QString &userID)
    : Account(ProviderId::Twitch)
    , oauthClient_(oauthClient)
    , oauthToken_(oauthToken)
    , userName_(username)
    , userId_(userID)
    , isAnon_(username == ANONYMOUS_USERNAME)
    , globallyAccessibleTwitchEmotes_(std::make_shared<EmoteMap>())
{
    // this is ugly
    auto *iTwitchEmotes = getIApp()->getEmotes()->getTwitchEmotes();
    auto *twitchEmotes = dynamic_cast<TwitchEmotes *>(iTwitchEmotes);
    assert(twitchEmotes);
    auto c = twitchEmotes->setsChanged.connect([this] {
        EmoteMap newEmoteMap;

        for (const auto &emoteSet : getApp()->emotes->twitch.twitchEmoteSets)
        {
            if (!this->globallyAccessibleEmoteSetIDs_.contains(emoteSet.first))
            {
                continue;
            }

            for (const auto &emote : emoteSet.second->emotes)
            {
                newEmoteMap.emplace(emote.first, emote.second);
            }
        }

        this->globallyAccessibleTwitchEmotes_.set(
            std::make_shared<EmoteMap>(newEmoteMap));
    });
    this->bSignals_.emplace_back(std::move(c));
}

QString TwitchAccount::toString() const
{
    return this->getUserName();
}

const QString &TwitchAccount::getUserName() const
{
    return this->userName_;
}

const QString &TwitchAccount::getOAuthClient() const
{
    return this->oauthClient_;
}

const QString &TwitchAccount::getOAuthToken() const
{
    return this->oauthToken_;
}

const QString &TwitchAccount::getUserId() const
{
    return this->userId_;
}

QColor TwitchAccount::color()
{
    return this->color_.get();
}

void TwitchAccount::setColor(QColor color)
{
    this->color_.set(std::move(color));
}

bool TwitchAccount::setOAuthClient(const QString &newClientID)
{
    if (this->oauthClient_.compare(newClientID) == 0)
    {
        return false;
    }

    this->oauthClient_ = newClientID;

    return true;
}

bool TwitchAccount::setOAuthToken(const QString &newOAuthToken)
{
    if (this->oauthToken_.compare(newOAuthToken) == 0)
    {
        return false;
    }

    this->oauthToken_ = newOAuthToken;

    return true;
}

bool TwitchAccount::isAnon() const
{
    return this->isAnon_;
}

void TwitchAccount::loadBlocks()
{
    getHelix()->loadBlocks(
        getIApp()->getAccounts()->twitch.getCurrent()->userId_,
        [this](std::vector<HelixBlock> blocks) {
            auto ignores = this->ignores_.access();
            auto userIds = this->ignoresUserIds_.access();
            ignores->clear();
            userIds->clear();

            for (const HelixBlock &block : blocks)
            {
                TwitchUser blockedUser;
                blockedUser.fromHelixBlock(block);
                ignores->insert(blockedUser);
                userIds->insert(blockedUser.id);
            }
        },
        [] {
            qCWarning(chatterinoTwitch) << "Fetching blocks failed!";
        });
}

void TwitchAccount::blockUser(QString userId, std::function<void()> onSuccess,
                              std::function<void()> onFailure)
{
    getHelix()->blockUser(
        userId,
        [this, userId, onSuccess] {
            TwitchUser blockedUser;
            blockedUser.id = userId;
            {
                auto ignores = this->ignores_.access();
                auto userIds = this->ignoresUserIds_.access();

                ignores->insert(blockedUser);
                userIds->insert(blockedUser.id);
            }
            onSuccess();
        },
        std::move(onFailure));
}

void TwitchAccount::unblockUser(QString userId, std::function<void()> onSuccess,
                                std::function<void()> onFailure)
{
    getHelix()->unblockUser(
        userId,
        [this, userId, onSuccess] {
            TwitchUser ignoredUser;
            ignoredUser.id = userId;
            {
                auto ignores = this->ignores_.access();
                auto userIds = this->ignoresUserIds_.access();

                ignores->erase(ignoredUser);
                userIds->erase(ignoredUser.id);
            }
            onSuccess();
        },
        std::move(onFailure));
}

SharedAccessGuard<const std::set<TwitchUser>> TwitchAccount::accessBlocks()
    const
{
    return this->ignores_.accessConst();
}

SharedAccessGuard<const std::set<QString>> TwitchAccount::accessBlockedUserIds()
    const
{
    return this->ignoresUserIds_.accessConst();
}

// AutoModActions
void TwitchAccount::autoModAllow(const QString msgID, ChannelPtr channel)
{
    getHelix()->manageAutoModMessages(
        this->getUserId(), msgID, "ALLOW",
        [] {
            // success
        },
        [channel](auto error) {
            // failure
            QString errorMessage("Failed to allow AutoMod message - ");

            switch (error)
            {
                case HelixAutoModMessageError::MessageAlreadyProcessed: {
                    errorMessage += "message has already been processed.";
                }
                break;

                case HelixAutoModMessageError::UserNotAuthenticated: {
                    errorMessage += "you need to re-authenticate.";
                }
                break;

                case HelixAutoModMessageError::UserNotAuthorized: {
                    errorMessage +=
                        "you don't have permission to perform that action";
                }
                break;

                case HelixAutoModMessageError::MessageNotFound: {
                    errorMessage += "target message not found.";
                }
                break;

                // This would most likely happen if the service is down, or if the JSON payload returned has changed format
                case HelixAutoModMessageError::Unknown:
                default: {
                    errorMessage += "an unknown error occured.";
                }
                break;
            }

            channel->addMessage(makeSystemMessage(errorMessage));
        });
}

void TwitchAccount::autoModDeny(const QString msgID, ChannelPtr channel)
{
    getHelix()->manageAutoModMessages(
        this->getUserId(), msgID, "DENY",
        [] {
            // success
        },
        [channel](auto error) {
            // failure
            QString errorMessage("Failed to deny AutoMod message - ");

            switch (error)
            {
                case HelixAutoModMessageError::MessageAlreadyProcessed: {
                    errorMessage += "message has already been processed.";
                }
                break;

                case HelixAutoModMessageError::UserNotAuthenticated: {
                    errorMessage += "you need to re-authenticate.";
                }
                break;

                case HelixAutoModMessageError::UserNotAuthorized: {
                    errorMessage +=
                        "you don't have permission to perform that action";
                }
                break;

                case HelixAutoModMessageError::MessageNotFound: {
                    errorMessage += "target message not found.";
                }
                break;

                // This would most likely happen if the service is down, or if the JSON payload returned has changed format
                case HelixAutoModMessageError::Unknown:
                default: {
                    errorMessage += "an unknown error occured.";
                }
                break;
            }

            channel->addMessage(makeSystemMessage(errorMessage));
        });
}

void TwitchAccount::setGlobalUserStateEmoteSetIDs(QStringList emoteSetIDs)
{
    qDebug() << "XXX: Set global user state emote set ids to" << emoteSetIDs;
    this->globallyAccessibleEmoteSetIDs_ = std::move(emoteSetIDs);

    getApp()->emotes->twitch.loadSets(this->globallyAccessibleEmoteSetIDs_);
}

QStringList TwitchAccount::globalUserStateEmoteSetIDs() const
{
    return this->globallyAccessibleEmoteSetIDs_;
}

std::shared_ptr<const EmoteMap> TwitchAccount::globallyAccessibleTwitchEmotes()
    const
{
    return this->globallyAccessibleTwitchEmotes_.get();
}

}  // namespace chatterino
