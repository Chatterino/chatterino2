#include "providers/twitch/TwitchAccount.hpp"

#include "providers/twitch/Const.hpp"
#include "debug/Log.hpp"
#include "common/NetworkRequest.hpp"
#include "util/RapidjsonHelpers.hpp"
#include "common/UrlFetch.hpp"

namespace chatterino {

TwitchAccount::TwitchAccount(const QString &_username, const QString &_oauthToken,
                             const QString &_oauthClient, const QString &_userID)
    : Account(ProviderId::Twitch)
    , oauthClient(_oauthClient)
    , oauthToken(_oauthToken)
    , userName(_username)
    , userId(_userID)
    , _isAnon(_username == ANONYMOUS_USERNAME)
{
}

QString TwitchAccount::toString() const
{
    return this->getUserName();
}

const QString &TwitchAccount::getUserName() const
{
    return this->userName;
}

const QString &TwitchAccount::getOAuthClient() const
{
    return this->oauthClient;
}

const QString &TwitchAccount::getOAuthToken() const
{
    return this->oauthToken;
}

const QString &TwitchAccount::getUserId() const
{
    return this->userId;
}

bool TwitchAccount::setOAuthClient(const QString &newClientID)
{
    if (this->oauthClient.compare(newClientID) == 0) {
        return false;
    }

    this->oauthClient = newClientID;

    return true;
}

bool TwitchAccount::setOAuthToken(const QString &newOAuthToken)
{
    if (this->oauthToken.compare(newOAuthToken) == 0) {
        return false;
    }

    this->oauthToken = newOAuthToken;

    return true;
}

bool TwitchAccount::isAnon() const
{
    return this->_isAnon;
}

void TwitchAccount::loadIgnores()
{
    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() + "/blocks");

    NetworkRequest req(url);
    req.setRequestType(NetworkRequest::GetRequest);
    req.setCaller(QThread::currentThread());
    req.makeAuthorizedV5(this->getOAuthClient(), this->getOAuthToken());
    req.onSuccess([=](const rapidjson::Document &document) {
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
            std::lock_guard<std::mutex> lock(this->ignoresMutex);
            this->ignores.clear();

            for (const auto &block : blocks.GetArray()) {
                if (!block.IsObject()) {
                    continue;
                }
                auto userIt = block.FindMember("user");
                if (userIt == block.MemberEnd()) {
                    continue;
                }
                this->ignores.insert(TwitchUser::fromJSON(userIt->value));
            }
        }

        return true;
    });

    req.execute();
}

void TwitchAccount::ignore(const QString &targetName,
                           std::function<void(IgnoreResult, const QString &)> onFinished)
{
    getUserID(targetName, QThread::currentThread(), [=](QString targetUserID) {
        this->ignoreByID(targetUserID, targetName, onFinished);  //
    });
}

void TwitchAccount::ignoreByID(const QString &targetUserID, const QString &targetName,
                               std::function<void(IgnoreResult, const QString &)> onFinished)
{
    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() + "/blocks/" +
                targetUserID);

    NetworkRequest req(url);
    req.setRequestType(NetworkRequest::PutRequest);
    req.setCaller(QThread::currentThread());
    req.makeAuthorizedV5(this->getOAuthClient(), this->getOAuthToken());

    req.onError([=](int errorCode) {
        onFinished(IgnoreResult_Failed, "An unknown error occured while trying to ignore user " +
                                            targetName + " (" + QString::number(errorCode) + ")");

        return true;
    });

    req.onSuccess([=](const rapidjson::Document &document) {
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

        auto ignoredUser = TwitchUser::fromJSON(userIt->value);
        {
            std::lock_guard<std::mutex> lock(this->ignoresMutex);

            auto res = this->ignores.insert(ignoredUser);
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
    getUserID(targetName, QThread::currentThread(), [=](QString targetUserID) {
        this->unignoreByID(targetUserID, targetName, onFinished);  //
    });
}

void TwitchAccount::unignoreByID(
    const QString &targetUserID, const QString &targetName,
    std::function<void(UnignoreResult, const QString &message)> onFinished)
{
    QString url("https://api.twitch.tv/kraken/users/" + this->getUserId() + "/blocks/" +
                targetUserID);

    NetworkRequest req(url);
    req.setRequestType(NetworkRequest::DeleteRequest);
    req.setCaller(QThread::currentThread());
    req.makeAuthorizedV5(this->getOAuthClient(), this->getOAuthToken());

    req.onError([=](int errorCode) {
        onFinished(UnignoreResult_Failed,
                   "An unknown error occured while trying to unignore user " + targetName + " (" +
                       QString::number(errorCode) + ")");

        return true;
    });

    req.onSuccess([=](const rapidjson::Document &document) {
        TwitchUser ignoredUser;
        ignoredUser.id = targetUserID;
        {
            std::lock_guard<std::mutex> lock(this->ignoresMutex);

            this->ignores.erase(ignoredUser);
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
    req.setRequestType(NetworkRequest::GetRequest);
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

    req.onSuccess([=](const rapidjson::Document &document) {
        onFinished(FollowResult_Following);
        return true;
    });

    req.execute();
}

std::set<TwitchUser> TwitchAccount::getIgnores() const
{
    std::lock_guard<std::mutex> lock(this->ignoresMutex);

    return this->ignores;
}

}  // namespace chatterino
