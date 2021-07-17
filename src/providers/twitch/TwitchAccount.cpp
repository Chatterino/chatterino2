#include "providers/twitch/TwitchAccount.hpp"

#include <QThread>

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/IvrApi.hpp"
#include "providers/irc/IrcMessageBuilder.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchUser.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/api/Kraken.hpp"
#include "singletons/Emotes.hpp"
#include "util/QStringHash.hpp"
#include "util/RapidjsonHelpers.hpp"

namespace {

std::vector<QStringList> getEmoteSetBatches(QStringList emoteSetKeys)
{
    // splitting emoteSetKeys to batches of 100, because Ivr API endpoint accepts a maximum of 100 emotesets at once
    constexpr int batchSize = 100;

    int batchCount = (emoteSetKeys.size() / batchSize) + 1;

    std::vector<QStringList> batches;
    batches.reserve(batchCount);

    for (int i = 0; i < batchCount; i++)
    {
        QStringList batch;

        int last = std::min(batchSize, emoteSetKeys.size() - batchSize * i);
        for (int j = 0; j < last; j++)
        {
            batch.push_back(emoteSetKeys.at(j + (batchSize * i)));
        }
        batches.emplace_back(batch);
    }

    return batches;
}

}  // namespace

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
    getHelix()->loadBlocks(
        getApp()->accounts->twitch.getCurrent()->userId_,
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

void TwitchAccount::checkFollow(const QString targetUserID,
                                std::function<void(FollowResult)> onFinished)
{
    const auto onResponse = [onFinished](bool following, const auto &record) {
        if (!following)
        {
            onFinished(FollowResult_NotFollowing);
            return;
        }

        onFinished(FollowResult_Following);
    };

    getHelix()->getUserFollow(this->getUserId(), targetUserID, onResponse,
                              [] {});
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

void TwitchAccount::loadEmotes()
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

    // TODO(zneix): Once Helix adds Get User Emotes we could remove this hacky solution
    // For now, this is necessary as Kraken's equivalent doesn't return all emotes
    // See: https://twitch.uservoice.com/forums/310213-developers/suggestions/43599900
    this->loadUserstateEmotes([=] {
        // Fill up emoteData with emote sets that were returned in a Kraken call, but aren't present in emoteData.
        this->loadKrakenEmotes();
    });
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

void TwitchAccount::loadUserstateEmotes(std::function<void()> callback)
{
    if (this->userstateEmoteSets_.isEmpty())
    {
        callback();
        return;
    }

    QStringList newEmoteSetKeys, krakenEmoteSetKeys;

    auto emoteData = this->emotes_.access();
    auto userEmoteSets = emoteData->emoteSets;

    // get list of already fetched emote sets
    for (const auto &userEmoteSet : userEmoteSets)
    {
        krakenEmoteSetKeys.push_back(userEmoteSet->key);
    }

    // filter out emote sets from userstate message, which are not in fetched emote set list
    for (const auto &emoteSetKey : qAsConst(this->userstateEmoteSets_))
    {
        if (!krakenEmoteSetKeys.contains(emoteSetKey))
        {
            newEmoteSetKeys.push_back(emoteSetKey);
        }
    }

    // return if there are no new emote sets
    if (newEmoteSetKeys.isEmpty())
    {
        callback();
        return;
    }

    // requesting emotes
    auto batches = getEmoteSetBatches(newEmoteSetKeys);
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
            [this](QJsonArray emoteSetArray) {
                auto emoteData = this->emotes_.access();
                auto localEmoteData = this->localEmotes_.access();
                for (auto emoteSet_ : emoteSetArray)
                {
                    auto emoteSet = std::make_shared<EmoteSet>();

                    IvrEmoteSet ivrEmoteSet(emoteSet_.toObject());

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

                    emoteSet->channelName = ivrEmoteSet.login;
                    emoteSet->text = ivrEmoteSet.displayName;

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
                        if (ivrEmote.emoteType == "FOLLOWER")
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
            },
            [] {
                // fetching emotes failed, ivr API might be down
            },
            [=] {
                // XXX(zneix): We check if this is the last iteration and if so, call the callback
                if (i + 1 == batches.size())
                {
                    qCDebug(chatterinoTwitch)
                        << "Finished loading emotes from IVR, attempting to "
                           "load Kraken emotes now";
                    callback();
                }
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

void TwitchAccount::loadKrakenEmotes()
{
    getKraken()->getUserEmotes(
        this,
        [this](KrakenEmoteSets data) {
            // no emotes available
            if (data.emoteSets.isEmpty())
            {
                qCWarning(chatterinoTwitch)
                    << "\"emoticon_sets\" either empty or not present in "
                       "Kraken::getUserEmotes response";
                return;
            }

            auto emoteData = this->emotes_.access();

            for (auto emoteSetIt = data.emoteSets.begin();
                 emoteSetIt != data.emoteSets.end(); ++emoteSetIt)
            {
                auto emoteSet = std::make_shared<EmoteSet>();

                QString setKey = emoteSetIt.key();
                emoteSet->key = setKey;
                this->loadEmoteSetData(emoteSet);

                // check if the emoteset is already in emoteData
                auto isAlreadyFetched = std::find_if(
                    emoteData->emoteSets.begin(), emoteData->emoteSets.end(),
                    [setKey](std::shared_ptr<EmoteSet> set) {
                        return (set->key == setKey);
                    });
                if (isAlreadyFetched != emoteData->emoteSets.end())
                {
                    continue;
                }

                for (const auto emoteArrObj : emoteSetIt->toArray())
                {
                    if (!emoteArrObj.isObject())
                    {
                        qCWarning(chatterinoTwitch)
                            << QString("Emote value from set %1 was invalid")
                                   .arg(emoteSet->key);
                        continue;
                    }
                    KrakenEmote krakenEmote(emoteArrObj.toObject());

                    auto id = EmoteId{krakenEmote.id};
                    auto code = EmoteName{
                        TwitchEmotes::cleanUpEmoteCode(krakenEmote.code)};

                    emoteSet->emotes.emplace_back(TwitchEmote{id, code});

                    if (!emoteSet->local)
                    {
                        auto emote =
                            getApp()->emotes->twitch.getOrCreateEmote(id, code);
                        emoteData->emotes.emplace(code, emote);
                    }
                }

                std::sort(emoteSet->emotes.begin(), emoteSet->emotes.end(),
                          [](const TwitchEmote &l, const TwitchEmote &r) {
                              return l.name.string < r.name.string;
                          });
                emoteData->emoteSets.emplace_back(emoteSet);
            }
        },
        [] {
            // kraken request failed
        });
}

void TwitchAccount::loadEmoteSetData(std::shared_ptr<EmoteSet> emoteSet)
{
    if (!emoteSet)
    {
        qCWarning(chatterinoTwitch) << "null emote set sent";
        return;
    }

    auto staticSetIt = this->staticEmoteSets.find(emoteSet->key);
    if (staticSetIt != this->staticEmoteSets.end())
    {
        const auto &staticSet = staticSetIt->second;
        emoteSet->channelName = staticSet.channelName;
        emoteSet->text = staticSet.text;
        return;
    }

    getHelix()->getEmoteSetData(
        emoteSet->key,
        [emoteSet](HelixEmoteSetData emoteSetData) {
            // Follower emotes can be only used in their origin channel
            if (emoteSetData.emoteType == "follower")
            {
                emoteSet->local = true;
            }

            if (emoteSetData.ownerId.isEmpty() ||
                emoteSetData.setId != emoteSet->key)
            {
                qCDebug(chatterinoTwitch)
                    << QString("Failed to fetch emoteSetData for %1, assuming "
                               "Twitch is the owner")
                           .arg(emoteSet->key);

                // most (if not all) emotes that fail to load are time limited event emotes owned by Twitch
                emoteSet->channelName = "twitch";
                emoteSet->text = "Twitch";

                return;
            }

            // emote set 0 = global emotes
            if (emoteSetData.ownerId == "0")
            {
                // emoteSet->channelName = QString();
                emoteSet->text = "Twitch Global";
                return;
            }

            getHelix()->getUserById(
                emoteSetData.ownerId,
                [emoteSet](HelixUser user) {
                    emoteSet->channelName = user.login;
                    emoteSet->text = user.displayName;
                },
                [emoteSetData] {
                    qCWarning(chatterinoTwitch)
                        << "Failed to query user by id:" << emoteSetData.ownerId
                        << emoteSetData.setId;
                });
        },
        [emoteSet] {
            // fetching emoteset data failed
            return;
        });
}

}  // namespace chatterino
