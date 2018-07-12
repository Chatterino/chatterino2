#include "providers/twitch/TwitchAccount.hpp"

#include "common/NetworkRequest.hpp"
#include "common/UrlFetch.hpp"
#include "debug/Log.hpp"
#include "providers/twitch/PartialTwitchUser.hpp"
#include "providers/twitch/TwitchCommon.hpp"
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
    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() + "/blocks");

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.makeAuthorizedV5(this->getOAuthClient(), this->getOAuthToken());
    req.onSuccess([=](auto result) {
        auto document = result.parseRapidJson();
        if (!document.IsObject()) {
            return false;
        }

        auto blocksIt = document.FindMember("blocks");
        if (blocksIt == document.MemberEnd()) {
            return false;
        }
        const auto &blocks = blocksIt->value;

        if (!blocks.IsArray()) {
            return false;
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
                    Log("Error parsing twitch user JSON {}", rj::stringify(userIt->value));
                    continue;
                }

                this->ignores_.insert(ignoredUser);
            }
        }

        return true;
    });

    req.execute();
}

void TwitchAccount::ignore(const QString &targetName,
                           std::function<void(IgnoreResult, const QString &)> onFinished)
{
    const auto onIdFetched = [this, targetName, onFinished](QString targetUserId) {
        this->ignoreByID(targetUserId, targetName, onFinished);  //
    };

    PartialTwitchUser::byName(targetName).getId(onIdFetched);
}

void TwitchAccount::ignoreByID(const QString &targetUserID, const QString &targetName,
                               std::function<void(IgnoreResult, const QString &)> onFinished)
{
    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() + "/blocks/" +
                targetUserID);

    NetworkRequest req(url, NetworkRequestType::Put);
    req.setCaller(QThread::currentThread());
    req.makeAuthorizedV5(this->getOAuthClient(), this->getOAuthToken());

    req.onError([=](int errorCode) {
        onFinished(IgnoreResult_Failed, "An unknown error occured while trying to ignore user " +
                                            targetName + " (" + QString::number(errorCode) + ")");

        return true;
    });

    req.onSuccess([=](auto result) {
        auto document = result.parseRapidJson();
        if (!document.IsObject()) {
            onFinished(IgnoreResult_Failed, "Bad JSON data while ignoring user " + targetName);
            return false;
        }

        auto userIt = document.FindMember("user");
        if (userIt == document.MemberEnd()) {
            onFinished(IgnoreResult_Failed,
                       "Bad JSON data while ignoring user (missing user) " + targetName);
            return false;
        }

        TwitchUser ignoredUser;
        if (!rj::getSafe(userIt->value, ignoredUser)) {
            onFinished(IgnoreResult_Failed,
                       "Bad JSON data while ignoring user (invalid user) " + targetName);
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(this->ignoresMutex_);

            auto res = this->ignores_.insert(ignoredUser);
            if (!res.second) {
                const TwitchUser &existingUser = *(res.first);
                existingUser.update(ignoredUser);
                onFinished(IgnoreResult_AlreadyIgnored,
                           "User " + targetName + " is already ignored");
                return false;
            }
        }
        onFinished(IgnoreResult_Success, "Successfully ignored user " + targetName);

        return true;
    });

    req.execute();
}

void TwitchAccount::unignore(const QString &targetName,
                             std::function<void(UnignoreResult, const QString &message)> onFinished)
{
    const auto onIdFetched = [this, targetName, onFinished](QString targetUserId) {
        this->unignoreByID(targetUserId, targetName, onFinished);  //
    };

    PartialTwitchUser::byName(targetName).getId(onIdFetched);
}

void TwitchAccount::unignoreByID(
    const QString &targetUserID, const QString &targetName,
    std::function<void(UnignoreResult, const QString &message)> onFinished)
{
    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() + "/blocks/" +
                targetUserID);

    NetworkRequest req(url, NetworkRequestType::Delete);
    req.setCaller(QThread::currentThread());
    req.makeAuthorizedV5(this->getOAuthClient(), this->getOAuthToken());

    req.onError([=](int errorCode) {
        onFinished(UnignoreResult_Failed,
                   "An unknown error occured while trying to unignore user " + targetName + " (" +
                       QString::number(errorCode) + ")");

        return true;
    });

    req.onSuccess([=](auto result) {
        auto document = result.parseRapidJson();
        TwitchUser ignoredUser;
        ignoredUser.id = targetUserID;
        {
            std::lock_guard<std::mutex> lock(this->ignoresMutex_);

            this->ignores_.erase(ignoredUser);
        }
        onFinished(UnignoreResult_Success, "Successfully unignored user " + targetName);

        return true;
    });

    req.execute();
}

void TwitchAccount::checkFollow(const QString targetUserID,
                                std::function<void(FollowResult)> onFinished)
{
    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() + "/follows/channels/" +
                targetUserID);

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

    req.onSuccess([=](auto result) {
        auto document = result.parseRapidJson();
        onFinished(FollowResult_Following);
        return true;
    });

    req.execute();
}

void TwitchAccount::followUser(const QString userID, std::function<void()> successCallback)
{
    QUrl requestUrl("https://api.twitch.tv/kraken/users/" + this->getUserId() +
                    "/follows/channels/" + userID);

    NetworkRequest request(requestUrl, NetworkRequestType::Put);
    request.setCaller(QThread::currentThread());

    request.makeAuthorizedV5(this->getOAuthClient(), this->getOAuthToken());

    // TODO: Properly check result of follow request
    request.onSuccess([successCallback](auto result) {
        successCallback();

        return true;
    });

    request.execute();
}

void TwitchAccount::unfollowUser(const QString userID, std::function<void()> successCallback)
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

    request.onSuccess([successCallback](const auto &document) {
        successCallback();

        return true;
    });

    request.execute();
}

std::set<TwitchUser> TwitchAccount::getIgnores() const
{
    std::lock_guard<std::mutex> lock(this->ignoresMutex_);

    return this->ignores_;
}

void TwitchAccount::loadEmotes(std::function<void(const rapidjson::Document &)> cb)
{
    Log("Loading Twitch emotes for user {}", this->getUserName());

    const auto &clientID = this->getOAuthClient();
    const auto &oauthToken = this->getOAuthToken();

    if (clientID.isEmpty() || oauthToken.isEmpty()) {
        Log("Missing Client ID or OAuth token");
        return;
    }

    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() + "/emotes");

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.makeAuthorizedV5(this->getOAuthClient(), this->getOAuthToken());

    req.onError([=](int errorCode) {
        Log("[TwitchAccount::loadEmotes] Error {}", errorCode);
        if (errorCode == 203) {
            // onFinished(FollowResult_NotFollowing);
        } else {
            // onFinished(FollowResult_Failed);
        }

        return true;
    });

    req.onSuccess([=](auto result) {
        cb(result.parseRapidJson());
        return true;
    });

    req.execute();
}

}  // namespace chatterino
