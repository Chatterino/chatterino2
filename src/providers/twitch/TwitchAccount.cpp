#include "providers/twitch/TwitchAccount.hpp"

#include <QThread>

#include "Application.hpp"
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "debug/Log.hpp"
#include "providers/twitch/PartialTwitchUser.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "singletons/Emotes.hpp"
#include "util/RapidjsonHelpers.hpp"

namespace chatterino {

namespace {

EmoteName cleanUpCode(const EmoteName &dirtyEmoteCode)
{
    auto cleanCode = dirtyEmoteCode.string;
    cleanCode.detach();

    static QMap<QString, QString> emoteNameReplacements{
        {"[oO](_|\\.)[oO]", "O_o"}, {"\\&gt\\;\\(", "&gt;("},
        {"\\&lt\\;3", "&lt;3"},     {"\\:-?(o|O)", ":O"},
        {"\\:-?(p|P)", ":P"},       {"\\:-?[\\\\/]", ":/"},
        {"\\:-?[z|Z|\\|]", ":Z"},   {"\\:-?\\(", ":("},
        {"\\:-?\\)", ":)"},         {"\\:-?D", ":D"},
        {"\\;-?(p|P)", ";P"},       {"\\;-?\\)", ";)"},
        {"R-?\\)", "R)"},           {"B-?\\)", "B)"},
    };

    auto it = emoteNameReplacements.find(dirtyEmoteCode.string);
    if (it != emoteNameReplacements.end()) {
        cleanCode = it.value();
    }

    cleanCode.replace("&lt;", "<");
    cleanCode.replace("&gt;", ">");

    return {cleanCode};
}

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
    this->color_.set(color);
}

bool TwitchAccount::setOAuthClient(const QString &newClientID)
{
    if (this->oauthClient_.compare(newClientID) == 0) {
        return false;
    }

    this->oauthClient_ = newClientID;

    return true;
}

bool TwitchAccount::setOAuthToken(const QString &newOAuthToken)
{
    if (this->oauthToken_.compare(newOAuthToken) == 0) {
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

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.makeAuthorizedV5(this->getOAuthClient(), this->getOAuthToken());
    req.onSuccess([=](auto result) -> Outcome {
        auto document = result.parseRapidJson();
        if (!document.IsObject()) {
            return Failure;
        }

        auto blocksIt = document.FindMember("blocks");
        if (blocksIt == document.MemberEnd()) {
            return Failure;
        }
        const auto &blocks = blocksIt->value;

        if (!blocks.IsArray()) {
            return Failure;
        }

        {
            std::lock_guard<std::mutex> lock(this->ignoresMutex_);
            this->ignores_.clear();

            for (const auto &block : blocks.GetArray()) {
                if (!block.IsObject()) {
                    continue;
                }
                auto userIt = block.FindMember("user");
                if (userIt == block.MemberEnd()) {
                    continue;
                }
                TwitchUser ignoredUser;
                if (!rj::getSafe(userIt->value, ignoredUser)) {
                    log("Error parsing twitch user JSON {}",
                        rj::stringify(userIt->value));
                    continue;
                }

                this->ignores_.insert(ignoredUser);
            }
        }

        return Success;
    });

    req.execute();
}

void TwitchAccount::ignore(
    const QString &targetName,
    std::function<void(IgnoreResult, const QString &)> onFinished)
{
    const auto onIdFetched = [this, targetName,
                              onFinished](QString targetUserId) {
        this->ignoreByID(targetUserId, targetName, onFinished);  //
    };

    PartialTwitchUser::byName(targetName).getId(onIdFetched);
}

void TwitchAccount::ignoreByID(
    const QString &targetUserID, const QString &targetName,
    std::function<void(IgnoreResult, const QString &)> onFinished)
{
    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() +
                "/blocks/" + targetUserID);
    NetworkRequest req(url, NetworkRequestType::Put);
    req.setCaller(QThread::currentThread());
    req.makeAuthorizedV5(this->getOAuthClient(), this->getOAuthToken());

    req.onError([=](int errorCode) {
        onFinished(IgnoreResult_Failed,
                   "An unknown error occured while trying to ignore user " +
                       targetName + " (" + QString::number(errorCode) + ")");

        return true;
    });

    req.onSuccess([=](auto result) -> Outcome {
        auto document = result.parseRapidJson();
        if (!document.IsObject()) {
            onFinished(IgnoreResult_Failed,
                       "Bad JSON data while ignoring user " + targetName);
            return Failure;
        }

        auto userIt = document.FindMember("user");
        if (userIt == document.MemberEnd()) {
            onFinished(IgnoreResult_Failed,
                       "Bad JSON data while ignoring user (missing user) " +
                           targetName);
            return Failure;
        }

        TwitchUser ignoredUser;
        if (!rj::getSafe(userIt->value, ignoredUser)) {
            onFinished(IgnoreResult_Failed,
                       "Bad JSON data while ignoring user (invalid user) " +
                           targetName);
            return Failure;
        }
        {
            std::lock_guard<std::mutex> lock(this->ignoresMutex_);

            auto res = this->ignores_.insert(ignoredUser);
            if (!res.second) {
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
    });

    req.execute();
}

void TwitchAccount::unignore(
    const QString &targetName,
    std::function<void(UnignoreResult, const QString &message)> onFinished)
{
    const auto onIdFetched = [this, targetName,
                              onFinished](QString targetUserId) {
        this->unignoreByID(targetUserId, targetName, onFinished);  //
    };

    PartialTwitchUser::byName(targetName).getId(onIdFetched);
}

void TwitchAccount::unignoreByID(
    const QString &targetUserID, const QString &targetName,
    std::function<void(UnignoreResult, const QString &message)> onFinished)
{
    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() +
                "/blocks/" + targetUserID);

    NetworkRequest req(url, NetworkRequestType::Delete);
    req.setCaller(QThread::currentThread());
    req.makeAuthorizedV5(this->getOAuthClient(), this->getOAuthToken());

    req.onError([=](int errorCode) {
        onFinished(UnignoreResult_Failed,
                   "An unknown error occured while trying to unignore user " +
                       targetName + " (" + QString::number(errorCode) + ")");

        return true;
    });

    req.onSuccess([=](auto result) -> Outcome {
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
    });

    req.execute();
}

void TwitchAccount::checkFollow(const QString targetUserID,
                                std::function<void(FollowResult)> onFinished)
{
    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() +
                "/follows/channels/" + targetUserID);

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.makeAuthorizedV5(this->getOAuthClient(), this->getOAuthToken());

    req.onError([=](int errorCode) {
        if (errorCode == 203) {
            onFinished(FollowResult_NotFollowing);
        } else {
            onFinished(FollowResult_Failed);
        }

        return true;
    });

    req.onSuccess([=](auto result) -> Outcome {
        auto document = result.parseRapidJson();
        onFinished(FollowResult_Following);
        return Success;
    });

    req.execute();
}

void TwitchAccount::followUser(const QString userID,
                               std::function<void()> successCallback)
{
    QUrl requestUrl("https://api.twitch.tv/kraken/users/" + this->getUserId() +
                    "/follows/channels/" + userID);

    NetworkRequest request(requestUrl, NetworkRequestType::Put);
    request.setCaller(QThread::currentThread());

    request.makeAuthorizedV5(this->getOAuthClient(), this->getOAuthToken());

    // TODO: Properly check result of follow request
    request.onSuccess([successCallback](auto result) -> Outcome {
        successCallback();

        return Success;
    });

    request.execute();
}

void TwitchAccount::unfollowUser(const QString userID,
                                 std::function<void()> successCallback)
{
    QUrl requestUrl("https://api.twitch.tv/kraken/users/" + this->getUserId() +
                    "/follows/channels/" + userID);

    NetworkRequest request(requestUrl, NetworkRequestType::Delete);
    request.setCaller(QThread::currentThread());

    request.makeAuthorizedV5(this->getOAuthClient(), this->getOAuthToken());

    request.onError([successCallback](int code) {
        if (code >= 200 && code <= 299) {
            successCallback();
        }

        return true;
    });

    request.onSuccess([successCallback](const auto &document) -> Outcome {
        successCallback();

        return Success;
    });

    request.execute();
}

std::set<TwitchUser> TwitchAccount::getIgnores() const
{
    std::lock_guard<std::mutex> lock(this->ignoresMutex_);

    return this->ignores_;
}

void TwitchAccount::loadEmotes()
{
    log("Loading Twitch emotes for user {}", this->getUserName());

    const auto &clientID = this->getOAuthClient();
    const auto &oauthToken = this->getOAuthToken();

    if (clientID.isEmpty() || oauthToken.isEmpty()) {
        log("Missing Client ID or OAuth token");
        return;
    }

    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() +
                "/emotes");

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.makeAuthorizedV5(this->getOAuthClient(), this->getOAuthToken());

    req.onError([=](int errorCode) {
        log("[TwitchAccount::loadEmotes] Error {}", errorCode);
        if (errorCode == 203) {
            // onFinished(FollowResult_NotFollowing);
        } else {
            // onFinished(FollowResult_Failed);
        }

        return true;
    });

    req.onSuccess([=](auto result) -> Outcome {
        this->parseEmotes(result.parseRapidJson());

        return Success;
    });

    req.execute();
}

AccessGuard<const TwitchAccount::TwitchAccountEmoteData>
TwitchAccount::accessEmotes() const
{
    return this->emotes_.accessConst();
}

void TwitchAccount::parseEmotes(const rapidjson::Document &root)
{
    auto emoteData = this->emotes_.access();

    emoteData->emoteSets.clear();
    emoteData->allEmoteNames.clear();

    auto emoticonSets = root.FindMember("emoticon_sets");
    if (emoticonSets == root.MemberEnd() || !emoticonSets->value.IsObject()) {
        log("No emoticon_sets in load emotes response");
        return;
    }

    for (const auto &emoteSetJSON : emoticonSets->value.GetObject()) {
        auto emoteSet = std::make_shared<EmoteSet>();

        emoteSet->key = emoteSetJSON.name.GetString();

        this->loadEmoteSetData(emoteSet);

        for (const rapidjson::Value &emoteJSON :
             emoteSetJSON.value.GetArray()) {
            if (!emoteJSON.IsObject()) {
                log("Emote value was invalid");
                return;
            }

            uint64_t idNumber;
            if (!rj::getSafe(emoteJSON, "id", idNumber)) {
                log("No ID key found in Emote value");
                return;
            }

            QString _code;
            if (!rj::getSafe(emoteJSON, "code", _code)) {
                log("No code key found in Emote value");
                return;
            }

            auto code = EmoteName{_code};
            auto id = EmoteId{QString::number(idNumber)};

            auto cleanCode = cleanUpCode(code);
            emoteSet->emotes.emplace_back(TwitchEmote{id, cleanCode});
            emoteData->allEmoteNames.push_back(cleanCode);

            auto emote = getApp()->emotes->twitch.getOrCreateEmote(id, code);
            emoteData->emotes.emplace(code, emote);
        }

        emoteData->emoteSets.emplace_back(emoteSet);
    }
};

void TwitchAccount::loadEmoteSetData(std::shared_ptr<EmoteSet> emoteSet)
{
    if (!emoteSet) {
        log("null emote set sent");
        return;
    }

    auto staticSetIt = this->staticEmoteSets.find(emoteSet->key);
    if (staticSetIt != this->staticEmoteSets.end()) {
        const auto &staticSet = staticSetIt->second;
        emoteSet->channelName = staticSet.channelName;
        emoteSet->text = staticSet.text;
        return;
    }

    NetworkRequest req(
        "https://braize.pajlada.com/chatterino/twitchemotes/set/" +
        emoteSet->key + "/");
    req.setUseQuickLoadCache(true);

    req.onError([](int errorCode) -> bool {
        log("Error code {} while loading emote set data", errorCode);
        return true;
    });

    req.onSuccess([emoteSet](auto result) -> Outcome {
        auto root = result.parseRapidJson();
        if (!root.IsObject()) {
            return Failure;
        }

        std::string emoteSetID;
        QString channelName;
        QString type;
        if (!rj::getSafe(root, "channel_name", channelName)) {
            return Failure;
        }

        if (!rj::getSafe(root, "type", type)) {
            return Failure;
        }

        log("Loaded twitch emote set data for {}!", emoteSet->key);

        auto name = channelName;
        name.detach();
        name[0] = name[0].toUpper();

        emoteSet->text = name;

        emoteSet->type = type;
        emoteSet->channelName = channelName;

        return Success;
    });

    req.execute();
}

}  // namespace chatterino
