#include "providers/twitch/api/Helix.hpp"

#include "common/Outcome.hpp"
#include "common/QLogging.hpp"

namespace chatterino {

static Helix *instance = nullptr;

void Helix::fetchUsers(QStringList userIds, QStringList userLogins,
                       ResultCallback<std::vector<HelixUser>> successCallback,
                       HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;

    for (const auto &id : userIds)
    {
        urlQuery.addQueryItem("id", id);
    }

    for (const auto &login : userLogins)
    {
        urlQuery.addQueryItem("login", login);
    }

    // TODO: set on success and on error
    this->makeRequest("users", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return Failure;
            }

            std::vector<HelixUser> users;

            for (const auto &jsonUser : data.toArray())
            {
                users.emplace_back(jsonUser.toObject());
            }

            successCallback(users);

            return Success;
        })
        .onError([failureCallback](auto result) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

void Helix::getUserByName(QString userName,
                          ResultCallback<HelixUser> successCallback,
                          HelixFailureCallback failureCallback)
{
    QStringList userIds;
    QStringList userLogins{userName};

    this->fetchUsers(
        userIds, userLogins,
        [successCallback,
         failureCallback](const std::vector<HelixUser> &users) {
            if (users.empty())
            {
                failureCallback();
                return;
            }
            successCallback(users[0]);
        },
        failureCallback);
}

void Helix::getUserById(QString userId,
                        ResultCallback<HelixUser> successCallback,
                        HelixFailureCallback failureCallback)
{
    QStringList userIds{userId};
    QStringList userLogins;

    this->fetchUsers(
        userIds, userLogins,
        [successCallback, failureCallback](const auto &users) {
            if (users.empty())
            {
                failureCallback();
                return;
            }
            successCallback(users[0]);
        },
        failureCallback);
}

void Helix::fetchUsersFollows(
    QString fromId, QString toId,
    ResultCallback<HelixUsersFollowsResponse> successCallback,
    HelixFailureCallback failureCallback)
{
    assert(!fromId.isEmpty() || !toId.isEmpty());

    QUrlQuery urlQuery;

    if (!fromId.isEmpty())
    {
        urlQuery.addQueryItem("from_id", fromId);
    }

    if (!toId.isEmpty())
    {
        urlQuery.addQueryItem("to_id", toId);
    }

    // TODO: set on success and on error
    this->makeRequest("users/follows", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            auto root = result.parseJson();
            if (root.empty())
            {
                failureCallback();
                return Failure;
            }
            successCallback(HelixUsersFollowsResponse(root));
            return Success;
        })
        .onError([failureCallback](auto result) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

void Helix::getUserFollowers(
    QString userId, ResultCallback<HelixUsersFollowsResponse> successCallback,
    HelixFailureCallback failureCallback)
{
    this->fetchUsersFollows("", userId, successCallback, failureCallback);
}

void Helix::getUserFollow(
    QString userId, QString targetId,
    ResultCallback<bool, HelixUsersFollowsRecord> successCallback,
    HelixFailureCallback failureCallback)
{
    this->fetchUsersFollows(
        userId, targetId,
        [successCallback](const auto &response) {
            if (response.data.empty())
            {
                successCallback(false, HelixUsersFollowsRecord());
                return;
            }

            successCallback(true, response.data[0]);
        },
        failureCallback);
}

void Helix::fetchStreams(
    QStringList userIds, QStringList userLogins,
    ResultCallback<std::vector<HelixStream>> successCallback,
    HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;

    for (const auto &id : userIds)
    {
        urlQuery.addQueryItem("user_id", id);
    }

    for (const auto &login : userLogins)
    {
        urlQuery.addQueryItem("user_login", login);
    }

    // TODO: set on success and on error
    this->makeRequest("streams", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return Failure;
            }

            std::vector<HelixStream> streams;

            for (const auto &jsonStream : data.toArray())
            {
                streams.emplace_back(jsonStream.toObject());
            }

            successCallback(streams);

            return Success;
        })
        .onError([failureCallback](auto result) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

void Helix::getStreamById(QString userId,
                          ResultCallback<bool, HelixStream> successCallback,
                          HelixFailureCallback failureCallback)
{
    QStringList userIds{userId};
    QStringList userLogins;

    this->fetchStreams(
        userIds, userLogins,
        [successCallback, failureCallback](const auto &streams) {
            if (streams.empty())
            {
                successCallback(false, HelixStream());
                return;
            }
            successCallback(true, streams[0]);
        },
        failureCallback);
}

void Helix::getStreamByName(QString userName,
                            ResultCallback<bool, HelixStream> successCallback,
                            HelixFailureCallback failureCallback)
{
    QStringList userIds;
    QStringList userLogins{userName};

    this->fetchStreams(
        userIds, userLogins,
        [successCallback, failureCallback](const auto &streams) {
            if (streams.empty())
            {
                successCallback(false, HelixStream());
                return;
            }
            successCallback(true, streams[0]);
        },
        failureCallback);
}

///

void Helix::fetchGames(QStringList gameIds, QStringList gameNames,
                       ResultCallback<std::vector<HelixGame>> successCallback,
                       HelixFailureCallback failureCallback)
{
    assert((gameIds.length() + gameNames.length()) > 0);

    QUrlQuery urlQuery;

    for (const auto &id : gameIds)
    {
        urlQuery.addQueryItem("id", id);
    }

    for (const auto &login : gameNames)
    {
        urlQuery.addQueryItem("name", login);
    }

    // TODO: set on success and on error
    this->makeRequest("games", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return Failure;
            }

            std::vector<HelixGame> games;

            for (const auto &jsonStream : data.toArray())
            {
                games.emplace_back(jsonStream.toObject());
            }

            successCallback(games);

            return Success;
        })
        .onError([failureCallback](auto result) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

void Helix::searchGames(QString query,
                        ResultCallback<std::vector<HelixGame>> successCallback,
                        HelixFailureCallback failureCallback)
{
    auto urlQuery = QUrlQuery();
    urlQuery.addQueryItem("query", query);
    this->makeRequest("search/categories", urlQuery)
        .onSuccess([successCallback,
                    failureCallback](NetworkResult result) -> Outcome {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return Failure;
            }

            std::vector<HelixGame> games;

            for (const auto &jsonStream : data.toArray())
            {
                games.emplace_back(jsonStream.toObject());
            }

            successCallback(games);
            return Success;
        })
        .onError([failureCallback](NetworkResult result) {
            qCDebug(chatterinoCommon) << "Something failed 4HEad";
            failureCallback();
        })
        .execute();
}

void Helix::updateStreamTags(QString broadcasterId, QStringList tags,
                             std::function<void()> successCallback,
                             HelixFailureCallback failureCallback)
{
    auto data = QJsonDocument();
    auto obj = QJsonObject();
    auto list = QJsonArray();
    for (QString tagId : tags)
    {
        list.append(tagId);
    }
    obj.insert("tag_ids", list);
    data.setObject(obj);
    qCDebug(chatterinoCommon) << data.toJson();

    auto urlQuery = QUrlQuery();
    urlQuery.addQueryItem("broadcaster_id", broadcasterId);
    this->makeRequest("streams/tags", urlQuery)
        .type(NetworkRequestType::Put)
        .payload(data.toJson())
        .header("Content-Type", "application/json")
        .onSuccess([successCallback,
                    failureCallback](NetworkResult result) -> Outcome {
            qCDebug(chatterinoCommon)
                << "Update tags success: " << result.status();
            successCallback();
            return Success;
        })
        .onError([successCallback, failureCallback](NetworkResult result) {
            qCDebug(chatterinoCommon)
                << "Update tags fail: " << result.status();
            failureCallback();
        })
        .execute();
}

void Helix::getStreamTags(QString broadcasterId,
                          ResultCallback<std::vector<HelixTag>> successCallback,
                          HelixFailureCallback failureCallback)
{
    auto urlQuery = QUrlQuery();
    urlQuery.addQueryItem("broadcaster_id", broadcasterId);
    this->makeRequest("streams/tags", urlQuery)
        .onSuccess([successCallback,
                    failureCallback](NetworkResult result) -> Outcome {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return Failure;
            }

            std::vector<HelixTag> tags;

            for (const auto &jsonTag : data.toArray())
            {
                tags.emplace_back(jsonTag.toObject());
            }

            successCallback(tags);
            return Success;
        })
        .onError([failureCallback](NetworkResult result) {
            qCDebug(chatterinoCommon) << result.status() << result.getData();
            failureCallback();
        })
        .execute();
}

void Helix::fetchStreamTags(
    QString after,
    ResultCallback<std::vector<HelixTag>, QString> successCallback,
    HelixFailureCallback failureCallback)
{
    auto urlQuery = QUrlQuery();
    if (!after.isNull() && !after.isEmpty())
    {
        urlQuery.addQueryItem("after", after);
    }
    urlQuery.addQueryItem("first", "100");
    this->makeRequest("tags/streams", urlQuery)
        .onSuccess([successCallback,
                    failureCallback](NetworkResult result) -> Outcome {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return Failure;
            }

            std::vector<HelixTag> tags;

            for (const auto &jsonTag : data.toArray())
            {
                tags.emplace_back(jsonTag.toObject());
            }

            successCallback(
                tags,
                root.value("pagination").toObject().value("cursor").toString());
            return Success;
        })
        .onError([failureCallback](NetworkResult result) {
            qCDebug(chatterinoCommon) << "status: " << result.status()
                                      << "data: " << result.getData();
            failureCallback();
        })
        .execute();
}

void Helix::getGameById(QString gameId,
                        ResultCallback<HelixGame> successCallback,
                        HelixFailureCallback failureCallback)
{
    QStringList gameIds{gameId};
    QStringList gameNames;

    this->fetchGames(
        gameIds, gameNames,
        [successCallback, failureCallback](const auto &games) {
            if (games.empty())
            {
                failureCallback();
                return;
            }
            successCallback(games[0]);
        },
        failureCallback);
}

void Helix::followUser(QString userId, QString targetId,
                       std::function<void()> successCallback,
                       HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;

    urlQuery.addQueryItem("from_id", userId);
    urlQuery.addQueryItem("to_id", targetId);

    this->makeRequest("users/follows", urlQuery)
        .type(NetworkRequestType::Post)
        .onSuccess([successCallback](auto result) -> Outcome {
            successCallback();
            return Success;
        })
        .onError([failureCallback](auto result) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

void Helix::unfollowUser(QString userId, QString targetId,
                         std::function<void()> successCallback,
                         HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;

    urlQuery.addQueryItem("from_id", userId);
    urlQuery.addQueryItem("to_id", targetId);

    this->makeRequest("users/follows", urlQuery)
        .type(NetworkRequestType::Delete)
        .onSuccess([successCallback](auto result) -> Outcome {
            successCallback();
            return Success;
        })
        .onError([failureCallback](auto result) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

void Helix::updateChannel(QString broadcasterId, QString gameId,
                          QString language, QString title,
                          std::function<void(NetworkResult)> successCallback,
                          HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;
    auto data = QJsonDocument();
    auto obj = QJsonObject();
    if (!gameId.isEmpty())
    {
        obj.insert("game_id", gameId);
    }
    if (!language.isEmpty())
    {
        obj.insert("broadcaster_language", language);
    }
    if (!title.isEmpty())
    {
        obj.insert("title", title);
    }

    if (title.isEmpty() && gameId.isEmpty() && language.isEmpty())
    {
        qCDebug(chatterinoCommon) << "Tried to update channel with no changes!";
        return;
    }

    data.setObject(obj);
    urlQuery.addQueryItem("broadcaster_id", broadcasterId);
    this->makeRequest("channels", urlQuery)
        .type(NetworkRequestType::Patch)
        .header("Content-Type", "application/json")
        .payload(data.toJson())
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            successCallback(result);
            return Success;
        })
        .onError([failureCallback](NetworkResult result) {
            failureCallback();
        })
        .execute();
}
NetworkRequest Helix::makeRequest(QString url, QUrlQuery urlQuery)
{
    assert(!url.startsWith("/"));

    if (this->clientId.isEmpty())
    {
        qCDebug(chatterinoTwitch)
            << "Helix::makeRequest called without a client ID set BabyRage";
        // return boost::none;
    }

    if (this->oauthToken.isEmpty())
    {
        qCDebug(chatterinoTwitch)
            << "Helix::makeRequest called without an oauth token set BabyRage";
        // return boost::none;
    }

    const QString baseUrl("https://api.twitch.tv/helix/");

    QUrl fullUrl(baseUrl + url);

    fullUrl.setQuery(urlQuery);

    return NetworkRequest(fullUrl)
        .timeout(5 * 1000)
        .header("Accept", "application/json")
        .header("Client-ID", this->clientId)
        .header("Authorization", "Bearer " + this->oauthToken);
}

void Helix::update(QString clientId, QString oauthToken)
{
    this->clientId = clientId;
    this->oauthToken = oauthToken;
}

void Helix::initialize()
{
    assert(instance == nullptr);

    instance = new Helix();
}

Helix *getHelix()
{
    assert(instance != nullptr);

    return instance;
}

}  // namespace chatterino
