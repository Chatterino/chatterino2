#include "providers/twitch/TwitchAccount.hpp"

#include <QThread>

#include "Application.hpp"
#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "qlogging.hpp"
#include "singletons/Emotes.hpp"
#include "util/RapidjsonHelpers.hpp"

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
    this->color_.set(color);
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

void TwitchAccount::loadIgnores()
{
    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() +
                "/blocks");

    NetworkRequest(url)

        .authorizeTwitchV5(this->getOAuthClient(), this->getOAuthToken())
        .onSuccess([=](auto result) -> Outcome {
            auto document = result.parseRapidJson();
            if (!document.IsObject())
            {
                return Failure;
            }

            auto blocksIt = document.FindMember("blocks");
            if (blocksIt == document.MemberEnd())
            {
                return Failure;
            }
            const auto &blocks = blocksIt->value;

            if (!blocks.IsArray())
            {
                return Failure;
            }

            {
                std::lock_guard<std::mutex> lock(this->ignoresMutex_);
                this->ignores_.clear();

                for (const auto &block : blocks.GetArray())
                {
                    if (!block.IsObject())
                    {
                        continue;
                    }
                    auto userIt = block.FindMember("user");
                    if (userIt == block.MemberEnd())
                    {
                        continue;
                    }
                    TwitchUser ignoredUser;
                    if (!rj::getSafe(userIt->value, ignoredUser))
                    {
                        qCWarning(chatterinoTwitch)
                            << "Error parsing twitch user JSON"
                            << rj::stringify(userIt->value).c_str();
                        continue;
                    }

                    this->ignores_.insert(ignoredUser);
                }
            }

            return Success;
        })
        .execute();
}

void TwitchAccount::ignore(
    const QString &targetName,
    std::function<void(IgnoreResult, const QString &)> onFinished)
{
    const auto onUserFetched = [this, targetName,
                                onFinished](const auto &user) {
        this->ignoreByID(user.id, targetName, onFinished);
    };

    const auto onUserFetchFailed = [] {};

    getHelix()->getUserByName(targetName, onUserFetched, onUserFetchFailed);
}

void TwitchAccount::ignoreByID(
    const QString &targetUserID, const QString &targetName,
    std::function<void(IgnoreResult, const QString &)> onFinished)
{
    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() +
                "/blocks/" + targetUserID);

    NetworkRequest(url, NetworkRequestType::Put)

        .authorizeTwitchV5(this->getOAuthClient(), this->getOAuthToken())
        .onError([=](NetworkResult result) {
            onFinished(IgnoreResult_Failed,
                       "An unknown error occured while trying to ignore user " +
                           targetName + " (" +
                           QString::number(result.status()) + ")");
        })
        .onSuccess([=](auto result) -> Outcome {
            auto document = result.parseRapidJson();
            if (!document.IsObject())
            {
                onFinished(IgnoreResult_Failed,
                           "Bad JSON data while ignoring user " + targetName);
                return Failure;
            }

            auto userIt = document.FindMember("user");
            if (userIt == document.MemberEnd())
            {
                onFinished(IgnoreResult_Failed,
                           "Bad JSON data while ignoring user (missing user) " +
                               targetName);
                return Failure;
            }

            TwitchUser ignoredUser;
            if (!rj::getSafe(userIt->value, ignoredUser))
            {
                onFinished(IgnoreResult_Failed,
                           "Bad JSON data while ignoring user (invalid user) " +
                               targetName);
                return Failure;
            }
            {
                std::lock_guard<std::mutex> lock(this->ignoresMutex_);

                auto res = this->ignores_.insert(ignoredUser);
                if (!res.second)
                {
                    const TwitchUser &existingUser = *(res.first);
                    existingUser.update(ignoredUser);
                    onFinished(IgnoreResult_AlreadyIgnored,
                               "User " + targetName + " is already ignored");
                    return Failure;
                }
            }
            onFinished(IgnoreResult_Success,
                       "Successfully ignored user " + targetName);

            return Success;
        })
        .execute();
}

void TwitchAccount::unignore(
    const QString &targetName,
    std::function<void(UnignoreResult, const QString &message)> onFinished)
{
    const auto onUserFetched = [this, targetName,
                                onFinished](const auto &user) {
        this->unignoreByID(user.id, targetName, onFinished);
    };

    const auto onUserFetchFailed = [] {};

    getHelix()->getUserByName(targetName, onUserFetched, onUserFetchFailed);
}

void TwitchAccount::unignoreByID(
    const QString &targetUserID, const QString &targetName,
    std::function<void(UnignoreResult, const QString &message)> onFinished)
{
    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() +
                "/blocks/" + targetUserID);

    NetworkRequest(url, NetworkRequestType::Delete)

        .authorizeTwitchV5(this->getOAuthClient(), this->getOAuthToken())
        .onError([=](NetworkResult result) {
            onFinished(
                UnignoreResult_Failed,
                "An unknown error occured while trying to unignore user " +
                    targetName + " (" + QString::number(result.status()) + ")");
        })
        .onSuccess([=](auto result) -> Outcome {
            auto document = result.parseRapidJson();
            TwitchUser ignoredUser;
            ignoredUser.id = targetUserID;
            {
                std::lock_guard<std::mutex> lock(this->ignoresMutex_);

                this->ignores_.erase(ignoredUser);
            }
            onFinished(UnignoreResult_Success,
                       "Successfully unignored user " + targetName);

            return Success;
        })
        .execute();
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

void TwitchAccount::followUser(const QString userID,
                               std::function<void()> successCallback)
{
    QUrl requestUrl("https://api.twitch.tv/kraken/users/" + this->getUserId() +
                    "/follows/channels/" + userID);

    NetworkRequest(requestUrl, NetworkRequestType::Put)

        .authorizeTwitchV5(this->getOAuthClient(), this->getOAuthToken())
        .onSuccess([successCallback](auto result) -> Outcome {
            // TODO: Properly check result of follow request
            successCallback();

            return Success;
        })
        .execute();
}

void TwitchAccount::unfollowUser(const QString userID,
                                 std::function<void()> successCallback)
{
    QUrl requestUrl("https://api.twitch.tv/kraken/users/" + this->getUserId() +
                    "/follows/channels/" + userID);

    NetworkRequest(requestUrl, NetworkRequestType::Delete)

        .authorizeTwitchV5(this->getOAuthClient(), this->getOAuthToken())
        .onError([successCallback](NetworkResult result) {
            if (result.status() >= 200 && result.status() <= 299)
            {
                successCallback();
            }
        })
        .onSuccess([successCallback](const auto &document) -> Outcome {
            successCallback();

            return Success;
        })
        .execute();
}

std::set<TwitchUser> TwitchAccount::getIgnores() const
{
    std::lock_guard<std::mutex> lock(this->ignoresMutex_);

    return this->ignores_;
}

void TwitchAccount::loadEmotes()
{
    qCDebug(chatterinoTwitch)
        << "Loading Twitch emotes for user" << this->getUserName();

    const auto &clientID = this->getOAuthClient();
    const auto &oauthToken = this->getOAuthToken();

    if (clientID.isEmpty() || oauthToken.isEmpty())
    {
        qCDebug(chatterinoTwitch) << "Missing Client ID or OAuth token";
        return;
    }

    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() +
                "/emotes");

    NetworkRequest(url)

        .authorizeTwitchV5(this->getOAuthClient(), this->getOAuthToken())
        .onError([=](NetworkResult result) {
            qCWarning(chatterinoTwitch)
                << "[TwitchAccount::loadEmotes] Error" << result.status();
            if (result.status() == 203)
            {
                // onFinished(FollowResult_NotFollowing);
            }
            else
            {
                // onFinished(FollowResult_Failed);
            }
        })
        .onSuccess([=](auto result) -> Outcome {
            this->parseEmotes(result.parseRapidJson());

            return Success;
        })
        .execute();
}

AccessGuard<const TwitchAccount::TwitchAccountEmoteData>
    TwitchAccount::accessEmotes() const
{
    return this->emotes_.accessConst();
}

// AutoModActions
void TwitchAccount::autoModAllow(const QString msgID)
{
    QString url("https://api.twitch.tv/kraken/chat/twitchbot/approve");

    auto qba = (QString("{\"msg_id\":\"") + msgID + "\"}").toUtf8();

    NetworkRequest(url, NetworkRequestType::Post)
        .header("Content-Type", "application/json")
        .header("Content-Length", QByteArray::number(qba.size()))
        .payload(qba)

        .authorizeTwitchV5(this->getOAuthClient(), this->getOAuthToken())
        .onError([=](NetworkResult result) {
            qCWarning(chatterinoTwitch)
                << "[TwitchAccounts::autoModAllow] Error" << result.status();
        })
        .execute();
}

void TwitchAccount::autoModDeny(const QString msgID)
{
    QString url("https://api.twitch.tv/kraken/chat/twitchbot/deny");

    auto qba = (QString("{\"msg_id\":\"") + msgID + "\"}").toUtf8();

    NetworkRequest(url, NetworkRequestType::Post)
        .header("Content-Type", "application/json")
        .header("Content-Length", QByteArray::number(qba.size()))
        .payload(qba)

        .authorizeTwitchV5(this->getOAuthClient(), this->getOAuthToken())
        .onError([=](NetworkResult result) {
            qCWarning(chatterinoTwitch)
                << "[TwitchAccounts::autoModDeny] Error" << result.status();
        })
        .execute();
}

void TwitchAccount::parseEmotes(const rapidjson::Document &root)
{
    auto emoteData = this->emotes_.access();

    emoteData->emoteSets.clear();
    emoteData->allEmoteNames.clear();

    auto emoticonSets = root.FindMember("emoticon_sets");
    if (emoticonSets == root.MemberEnd() || !emoticonSets->value.IsObject())
    {
        qCWarning(chatterinoTwitch)
            << "No emoticon_sets in load emotes response";
        return;
    }

    for (const auto &emoteSetJSON : emoticonSets->value.GetObject())
    {
        auto emoteSet = std::make_shared<EmoteSet>();

        emoteSet->key = emoteSetJSON.name.GetString();

        this->loadEmoteSetData(emoteSet);

        for (const rapidjson::Value &emoteJSON : emoteSetJSON.value.GetArray())
        {
            if (!emoteJSON.IsObject())
            {
                qCWarning(chatterinoTwitch) << "Emote value was invalid";
                return;
            }

            uint64_t idNumber;
            if (!rj::getSafe(emoteJSON, "id", idNumber))
            {
                qCWarning(chatterinoTwitch) << "No ID key found in Emote value";
                return;
            }

            QString _code;
            if (!rj::getSafe(emoteJSON, "code", _code))
            {
                qCWarning(chatterinoTwitch)
                    << "No code key found in Emote value";
                return;
            }

            auto code = EmoteName{_code};
            auto id = EmoteId{QString::number(idNumber)};

            auto cleanCode = EmoteName{TwitchEmotes::cleanUpEmoteCode(code)};
            emoteSet->emotes.emplace_back(TwitchEmote{id, cleanCode});
            emoteData->allEmoteNames.push_back(cleanCode);

            auto emote = getApp()->emotes->twitch.getOrCreateEmote(id, code);
            emoteData->emotes.emplace(code, emote);
        }

        std::sort(emoteSet->emotes.begin(), emoteSet->emotes.end(),
                  [](const TwitchEmote &l, const TwitchEmote &r) {
                      return l.name.string < r.name.string;
                  });
        emoteData->emoteSets.emplace_back(emoteSet);
    }
};

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
        .onError([](NetworkResult result) {
            qCWarning(chatterinoTwitch) << "Error code" << result.status()
                                        << "while loading emote set data";
        })
        .onSuccess([emoteSet](auto result) -> Outcome {
            auto root = result.parseRapidJson();
            if (!root.IsObject())
            {
                return Failure;
            }

            std::string emoteSetID;
            QString channelName;
            QString type;
            if (!rj::getSafe(root, "channel_name", channelName))
            {
                return Failure;
            }

            if (!rj::getSafe(root, "type", type))
            {
                return Failure;
            }

            qCDebug(chatterinoTwitch)
                << "Loaded twitch emote set data for" << emoteSet->key;

            auto name = channelName;
            name.detach();
            name[0] = name[0].toUpper();

            emoteSet->text = name;

            emoteSet->type = type;
            emoteSet->channelName = channelName;

            return Success;
        })
        .execute();
}

}  // namespace chatterino
