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
#include "util/RapidjsonHelpers.hpp"

namespace chatterino {
namespace {
    constexpr int USERSTATE_EMOTES_REFRESH_PERIOD = 10 * 60 * 1000;
}  // namespace

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
            qDebug() << "Fetching blocks failed!";
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
        qCDebug(chatterinoTwitch) << "Missing Client ID or OAuth token";
        return;
    }

    getKraken()->getUserEmotes(
        this,
        [this](KrakenEmoteSets data) {
            // clear emote data
            auto emoteData = this->emotes_.access();
            emoteData->emoteSets.clear();
            emoteData->allEmoteNames.clear();

            // no emotes available
            if (data.emoteSets.isEmpty())
            {
                qCWarning(chatterinoTwitch)
                    << "\"emoticon_sets\" either empty or not present in "
                       "Kraken::getUserEmotes response";
                return;
            }

            for (auto emoteSetIt = data.emoteSets.begin();
                 emoteSetIt != data.emoteSets.end(); ++emoteSetIt)
            {
                auto emoteSet = std::make_shared<EmoteSet>();

                emoteSet->key = emoteSetIt.key();
                this->loadEmoteSetData(emoteSet);

                for (const auto emoteArrObj : emoteSetIt.value().toArray())
                {
                    if (!emoteArrObj.isObject())
                    {
                        qCWarning(chatterinoTwitch)
                            << QString("Emote value from set %1 was invalid")
                                   .arg(emoteSet->key);
                    }
                    KrakenEmote krakenEmote(emoteArrObj.toObject());

                    auto id = EmoteId{krakenEmote.id};
                    auto code = EmoteName{krakenEmote.code};

                    auto cleanCode =
                        EmoteName{TwitchEmotes::cleanUpEmoteCode(code)};
                    emoteSet->emotes.emplace_back(TwitchEmote{id, cleanCode});
                    emoteData->allEmoteNames.push_back(cleanCode);

                    auto emote =
                        getApp()->emotes->twitch.getOrCreateEmote(id, code);
                    emoteData->emotes.emplace(code, emote);
                }

                std::sort(emoteSet->emotes.begin(), emoteSet->emotes.end(),
                          [](const TwitchEmote &l, const TwitchEmote &r) {
                              return l.name.string < r.name.string;
                          });
                emoteData->emoteSets.emplace_back(emoteSet);
            }
        },
        [] {
            // request failed
        });
}

void TwitchAccount::loadUserstateEmotes(QStringList emoteSetKeys)
{
    // do not attempt to load emotes too often
    if (!this->userstateEmotesTimer_.isValid())
    {
        this->userstateEmotesTimer_.start();
    }
    else if (this->userstateEmotesTimer_.elapsed() <
             USERSTATE_EMOTES_REFRESH_PERIOD)
    {
        return;
    }
    this->userstateEmotesTimer_.restart();

    auto emoteData = this->emotes_.access();
    auto userEmoteSets = emoteData->emoteSets;

    QStringList newEmoteSetKeys, currentEmoteSetKeys;
    // get list of already fetched emote sets
    for (const auto &userEmoteSet : userEmoteSets)
    {
        currentEmoteSetKeys.push_back(userEmoteSet->key);
    }
    // filter out emote sets from userstate message, which are not in fetched emote set list
    for (const auto &emoteSetKey : emoteSetKeys)
    {
        if (!currentEmoteSetKeys.contains(emoteSetKey))
        {
            newEmoteSetKeys.push_back(emoteSetKey);
        }
    }

    // return if there are no new emote sets
    if (newEmoteSetKeys.isEmpty())
    {
        return;
    }

    getIvr()->getBulkEmoteSets(
        newEmoteSetKeys.join(","),
        [this](QJsonArray emoteSetArray) {
            auto emoteData = this->emotes_.access();
            for (auto emoteSet : emoteSetArray)
            {
                auto newUserEmoteSet = std::make_shared<EmoteSet>();

                IvrEmoteSet ivrEmoteSet(emoteSet.toObject());

                newUserEmoteSet->key = ivrEmoteSet.setId;

                auto name = ivrEmoteSet.login;
                name.detach();
                name[0] = name[0].toUpper();

                newUserEmoteSet->text = name;
                newUserEmoteSet->type = QString();
                newUserEmoteSet->channelName = ivrEmoteSet.login;

                for (const auto &emote : ivrEmoteSet.emotes)
                {
                    IvrEmote ivrEmote(emote.toObject());

                    auto id = EmoteId{ivrEmote.id};
                    auto code = EmoteName{ivrEmote.code};
                    auto cleanCode =
                        EmoteName{TwitchEmotes::cleanUpEmoteCode(code)};
                    newUserEmoteSet->emotes.emplace_back(
                        TwitchEmote{id, cleanCode});

                    emoteData->allEmoteNames.push_back(cleanCode);

                    auto twitchEmote =
                        getApp()->emotes->twitch.getOrCreateEmote(id, code);
                    emoteData->emotes.emplace(code, twitchEmote);
                }
                std::sort(newUserEmoteSet->emotes.begin(),
                          newUserEmoteSet->emotes.end(),
                          [](const TwitchEmote &l, const TwitchEmote &r) {
                              return l.name.string < r.name.string;
                          });
                emoteData->emoteSets.emplace_back(newUserEmoteSet);
            }
        },
        [] {
            // fetching emotes failed, ivr API might be down
        });
}

SharedAccessGuard<const TwitchAccount::TwitchAccountEmoteData>
    TwitchAccount::accessEmotes() const
{
    return this->emotes_.accessConst();
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

    NetworkRequest(Env::get().twitchEmoteSetResolverUrl.arg(emoteSet->key))
        .cache()
        .onSuccess([emoteSet](NetworkResult result) -> Outcome {
            auto rootOld = result.parseRapidJson();
            auto root = result.parseJson();
            if (root.isEmpty())
            {
                return Failure;
            }

            TwitchEmoteSetResolverResponse response(root);

            auto name = response.channelName;
            name.detach();
            name[0] = name[0].toUpper();

            emoteSet->text = name;
            emoteSet->type = response.type;
            emoteSet->channelName = response.channelName;

            qCDebug(chatterinoTwitch)
                << QString("Loaded twitch emote set data for %1")
                       .arg(emoteSet->key);

            return Success;
        })
        .onError([](NetworkResult result) {
            qCWarning(chatterinoTwitch)
                << QString("Error code %1 while loading emote set data")
                       .arg(result.status());
        })
        .execute();
}

}  // namespace chatterino
