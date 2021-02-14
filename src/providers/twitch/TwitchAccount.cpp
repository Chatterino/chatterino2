#include "providers/twitch/TwitchAccount.hpp"

#include <QThread>

#include "Application.hpp"
#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchUser.hpp"
#include "providers/twitch/api/Helix.hpp"
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

void TwitchAccount::loadBlocks()
{
    getHelix()->loadBlocks(
        getApp()->accounts->twitch.getCurrent()->userId_,
        [this](std::vector<HelixBlock> blocks) {
            std::lock_guard<std::mutex> lock(this->ignoresMutex_);
            this->ignores_.clear();

            for (const HelixBlock &block : blocks)
            {
                TwitchUser blockedUser;
                blockedUser.fromHelixBlock(block);
                this->ignores_.insert(blockedUser);
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
                std::lock_guard<std::mutex> lock(this->ignoresMutex_);

                this->ignores_.insert(blockedUser);
            }
            onSuccess();
        },
        onFailure);
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
                std::lock_guard<std::mutex> lock(this->ignoresMutex_);

                this->ignores_.erase(ignoredUser);
            }
            onSuccess();
        },
        onFailure);
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

std::set<TwitchUser> TwitchAccount::getBlocks() const
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
