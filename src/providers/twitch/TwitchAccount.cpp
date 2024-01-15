#include "providers/twitch/TwitchAccount.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/Env.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/irc/IrcMessageBuilder.hpp"
#include "providers/IvrApi.hpp"
#include "providers/seventv/SeventvAPI.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "singletons/Emotes.hpp"
#include "util/CancellationToken.hpp"
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
{
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
    assertInGuiThread();

    auto token = CancellationToken(false);
    this->blockToken_ = token;
    this->ignores_.clear();
    this->ignoresUserIds_.clear();

    getHelix()->loadBlocks(
        getIApp()->getAccounts()->twitch.getCurrent()->userId_,
        [this](const std::vector<HelixBlock> &blocks) {
            assertInGuiThread();

            for (const HelixBlock &block : blocks)
            {
                TwitchUser blockedUser;
                blockedUser.fromHelixBlock(block);
                this->ignores_.insert(blockedUser);
                this->ignoresUserIds_.insert(blockedUser.id);
            }
        },
        [](auto error) {
            qCWarning(chatterinoTwitch).noquote()
                << "Fetching blocks failed:" << error;
        },
        std::move(token));
}

void TwitchAccount::blockUser(const QString &userId, const QObject *caller,
                              std::function<void()> onSuccess,
                              std::function<void()> onFailure)
{
    getHelix()->blockUser(
        userId, caller,
        [this, userId, onSuccess = std::move(onSuccess)] {
            assertInGuiThread();

            TwitchUser blockedUser;
            blockedUser.id = userId;
            this->ignores_.insert(blockedUser);
            this->ignoresUserIds_.insert(blockedUser.id);
            onSuccess();
        },
        std::move(onFailure));
}

void TwitchAccount::unblockUser(const QString &userId, const QObject *caller,
                                std::function<void()> onSuccess,
                                std::function<void()> onFailure)
{
    getHelix()->unblockUser(
        userId, caller,
        [this, userId, onSuccess = std::move(onSuccess)] {
            assertInGuiThread();

            TwitchUser ignoredUser;
            ignoredUser.id = userId;
            this->ignores_.erase(ignoredUser);
            this->ignoresUserIds_.erase(ignoredUser.id);
            onSuccess();
        },
        std::move(onFailure));
}

const std::unordered_set<TwitchUser> &TwitchAccount::blocks() const
{
    assertInGuiThread();
    return this->ignores_;
}

const std::unordered_set<QString> &TwitchAccount::blockedUserIds() const
{
    assertInGuiThread();
    return this->ignoresUserIds_;
}

void TwitchAccount::loadEmotes(std::weak_ptr<Channel> weakChannel)
{
    qCDebug(chatterinoTwitch)
        << "Loading Twitch emotes for user" << this->getUserName();

    if (this->getOAuthClient().isEmpty() || this->getOAuthToken().isEmpty())
    {
        qCDebug(chatterinoTwitch)
            << "Aborted loadEmotes due to missing Client ID and/or OAuth token";
        return;
    }

    {
        auto emoteData = this->emotes_.access();
        emoteData->emoteSets.clear();
        emoteData->emotes.clear();
        qCDebug(chatterinoTwitch) << "Cleared emotes!";
    }

    this->loadUserstateEmotes(weakChannel);
}

bool TwitchAccount::setUserstateEmoteSets(QStringList newEmoteSets)
{
    newEmoteSets.sort();

    if (this->userstateEmoteSets_ == newEmoteSets)
    {
        // Nothing has changed
        return false;
    }

    this->userstateEmoteSets_ = newEmoteSets;

    return true;
}

void TwitchAccount::loadUserstateEmotes(std::weak_ptr<Channel> weakChannel)
{
    if (this->userstateEmoteSets_.isEmpty())
    {
        return;
    }

    QStringList newEmoteSetKeys, existingEmoteSetKeys;

    auto emoteData = this->emotes_.access();
    auto userEmoteSets = emoteData->emoteSets;

    // get list of already fetched emote sets
    for (const auto &userEmoteSet : userEmoteSets)
    {
        existingEmoteSetKeys.push_back(userEmoteSet->key);
    }

    // filter out emote sets from userstate message, which are not in fetched emote set list
    for (const auto &emoteSetKey : qAsConst(this->userstateEmoteSets_))
    {
        if (!existingEmoteSetKeys.contains(emoteSetKey))
        {
            newEmoteSetKeys.push_back(emoteSetKey);
        }
    }

    // return if there are no new emote sets
    if (newEmoteSetKeys.isEmpty())
    {
        return;
    }

    // requesting emotes
    auto batches = splitListIntoBatches(newEmoteSetKeys);
    for (int i = 0; i < batches.size(); i++)
    {
        qCDebug(chatterinoTwitch)
            << QString(
                   "Loading %1 emotesets from IVR; batch %2/%3 (%4 sets): %5")
                   .arg(newEmoteSetKeys.size())
                   .arg(i + 1)
                   .arg(batches.size())
                   .arg(batches.at(i).size())
                   .arg(batches.at(i).join(","));
        getIvr()->getBulkEmoteSets(
            batches.at(i).join(","),
            [this, weakChannel](QJsonArray emoteSetArray) {
                auto emoteData = this->emotes_.access();
                auto localEmoteData = this->localEmotes_.access();

                std::unordered_set<QString> subscriberChannelIDs;
                std::vector<IvrEmoteSet> ivrEmoteSets;
                ivrEmoteSets.reserve(emoteSetArray.size());

                for (auto emoteSet : emoteSetArray)
                {
                    IvrEmoteSet ivrEmoteSet(emoteSet.toObject());
                    if (!ivrEmoteSet.tier.isNull())
                    {
                        subscriberChannelIDs.insert(ivrEmoteSet.channelId);
                    }
                    ivrEmoteSets.emplace_back(ivrEmoteSet);
                }

                for (const auto &emoteSet : emoteData->emoteSets)
                {
                    if (emoteSet->subscriber)
                    {
                        subscriberChannelIDs.insert(emoteSet->channelID);
                    }
                }

                for (const auto &ivrEmoteSet : ivrEmoteSets)
                {
                    auto emoteSet = std::make_shared<EmoteSet>();

                    QString setKey = ivrEmoteSet.setId;
                    emoteSet->key = setKey;

                    // check if the emoteset is already in emoteData
                    auto isAlreadyFetched =
                        std::find_if(emoteData->emoteSets.begin(),
                                     emoteData->emoteSets.end(),
                                     [setKey](std::shared_ptr<EmoteSet> set) {
                                         return (set->key == setKey);
                                     });
                    if (isAlreadyFetched != emoteData->emoteSets.end())
                    {
                        continue;
                    }

                    emoteSet->channelID = ivrEmoteSet.channelId;
                    emoteSet->channelName = ivrEmoteSet.login;
                    emoteSet->text = ivrEmoteSet.displayName;
                    emoteSet->subscriber = !ivrEmoteSet.tier.isNull();

                    // NOTE: If a user does not have a subscriber emote set, but a follower emote set, this logic will be wrong
                    // However, that's not a realistic problem.
                    bool haveSubscriberSetForChannel =
                        subscriberChannelIDs.contains(ivrEmoteSet.channelId);

                    for (const auto &emoteObj : ivrEmoteSet.emotes)
                    {
                        IvrEmote ivrEmote(emoteObj.toObject());

                        auto id = EmoteId{ivrEmote.id};
                        auto code = EmoteName{
                            TwitchEmotes::cleanUpEmoteCode(ivrEmote.code)};

                        emoteSet->emotes.push_back(TwitchEmote{id, code});

                        auto emote =
                            getApp()->emotes->twitch.getOrCreateEmote(id, code);

                        // Follower emotes can be only used in their origin channel
                        // unless the user is subscribed, then they can be used anywhere.
                        if (ivrEmote.emoteType == "FOLLOWER" &&
                            !haveSubscriberSetForChannel)
                        {
                            emoteSet->local = true;

                            // EmoteMap for target channel wasn't initialized yet, doing it now
                            if (localEmoteData->find(ivrEmoteSet.channelId) ==
                                localEmoteData->end())
                            {
                                localEmoteData->emplace(ivrEmoteSet.channelId,
                                                        EmoteMap());
                            }

                            localEmoteData->at(ivrEmoteSet.channelId)
                                .emplace(code, emote);
                        }
                        else
                        {
                            emoteData->emotes.emplace(code, emote);
                        }
                    }
                    std::sort(emoteSet->emotes.begin(), emoteSet->emotes.end(),
                              [](const TwitchEmote &l, const TwitchEmote &r) {
                                  return l.name.string < r.name.string;
                              });
                    emoteData->emoteSets.emplace_back(emoteSet);
                }

                if (auto channel = weakChannel.lock(); channel != nullptr)
                {
                    channel->addMessage(makeSystemMessage(
                        "Twitch subscriber emotes reloaded."));
                }
            },
            [] {
                // fetching emotes failed, ivr API might be down
            });
    };
}

SharedAccessGuard<const TwitchAccount::TwitchAccountEmoteData>
    TwitchAccount::accessEmotes() const
{
    return this->emotes_.accessConst();
}

SharedAccessGuard<const std::unordered_map<QString, EmoteMap>>
    TwitchAccount::accessLocalEmotes() const
{
    return this->localEmotes_.accessConst();
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

const QString &TwitchAccount::getSeventvUserID() const
{
    return this->seventvUserID_;
}

void TwitchAccount::loadSeventvUserID()
{
    if (this->isAnon())
    {
        return;
    }
    if (!this->seventvUserID_.isEmpty())
    {
        return;
    }

    auto *seventv = getIApp()->getSeventvAPI();
    if (!seventv)
    {
        qCWarning(chatterinoSeventv)
            << "Not loading 7TV User ID because the 7TV API is not initialized";
        return;
    }

    seventv->getUserByTwitchID(
        this->getUserId(),
        [this](const auto &json) {
            const auto id = json["user"]["id"].toString();
            if (!id.isEmpty())
            {
                this->seventvUserID_ = id;
            }
        },
        [](const auto &result) {
            qCDebug(chatterinoSeventv)
                << "Failed to load 7TV user-id:" << result.formatError();
        });
}

}  // namespace chatterino
