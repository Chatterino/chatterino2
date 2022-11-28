#include "providers/twitch/api/Helix.hpp"

#include "common/Outcome.hpp"
#include "common/QLogging.hpp"

#include <magic_enum.hpp>
#include <QJsonDocument>

namespace {

using namespace chatterino;

static constexpr auto NUM_MODERATORS_TO_FETCH_PER_REQUEST = 100;

static constexpr auto NUM_CHATTERS_TO_FETCH = 1000;

}  // namespace

namespace chatterino {

static IHelix *instance = nullptr;

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
        .onError([failureCallback](auto /*result*/) {
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
    QStringList userLogins{std::move(userName)};

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
    QStringList userIds{std::move(userId)};
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
        .onError([failureCallback](auto /*result*/) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

void Helix::getUserFollowers(
    QString userId, ResultCallback<HelixUsersFollowsResponse> successCallback,
    HelixFailureCallback failureCallback)
{
    this->fetchUsersFollows("", std::move(userId), std::move(successCallback),
                            std::move(failureCallback));
}

void Helix::fetchStreams(
    QStringList userIds, QStringList userLogins,
    ResultCallback<std::vector<HelixStream>> successCallback,
    HelixFailureCallback failureCallback, std::function<void()> finallyCallback)
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
        .onError([failureCallback](auto /*result*/) {
            // TODO: make better xd
            failureCallback();
        })
        .finally(finallyCallback)
        .execute();
}

void Helix::getStreamById(QString userId,
                          ResultCallback<bool, HelixStream> successCallback,
                          HelixFailureCallback failureCallback,
                          std::function<void()> finallyCallback)
{
    QStringList userIds{std::move(userId)};
    QStringList userLogins;

    this->fetchStreams(
        userIds, userLogins,
        [successCallback](const auto &streams) {
            if (streams.empty())
            {
                successCallback(false, HelixStream());
                return;
            }
            successCallback(true, streams[0]);
        },
        failureCallback, finallyCallback);
}

void Helix::getStreamByName(QString userName,
                            ResultCallback<bool, HelixStream> successCallback,
                            HelixFailureCallback failureCallback,
                            std::function<void()> finallyCallback)
{
    QStringList userIds;
    QStringList userLogins{std::move(userName)};

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
        failureCallback, finallyCallback);
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
        .onError([failureCallback](auto /*result*/) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

void Helix::searchGames(QString gameName,
                        ResultCallback<std::vector<HelixGame>> successCallback,
                        HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("query", gameName);

    this->makeRequest("search/categories", urlQuery)
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
        .onError([failureCallback](auto /*result*/) {
            // TODO: make better xd
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
    QStringList gameIds{std::move(gameId)};
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

void Helix::createClip(QString channelId,
                       ResultCallback<HelixClip> successCallback,
                       std::function<void(HelixClipError)> failureCallback,
                       std::function<void()> finallyCallback)
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("broadcaster_id", channelId);

    this->makeRequest("clips", urlQuery)
        .type(NetworkRequestType::Post)
        .header("Content-Type", "application/json")
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback(HelixClipError::Unknown);
                return Failure;
            }

            HelixClip clip(data.toArray()[0].toObject());

            successCallback(clip);
            return Success;
        })
        .onError([failureCallback](auto result) {
            switch (result.status())
            {
                case 503: {
                    // Channel has disabled clip-creation, or channel has made cliops only creatable by followers and the user is not a follower (or subscriber)
                    failureCallback(HelixClipError::ClipsDisabled);
                }
                break;

                case 401: {
                    // User does not have the required scope to be able to create clips, user must reauthenticate
                    failureCallback(HelixClipError::UserNotAuthenticated);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Failed to create a clip: " << result.status()
                        << result.getData();
                    failureCallback(HelixClipError::Unknown);
                }
                break;
            }
        })
        .finally(std::move(finallyCallback))
        .execute();
}

void Helix::getChannel(QString broadcasterId,
                       ResultCallback<HelixChannel> successCallback,
                       HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("broadcaster_id", broadcasterId);

    this->makeRequest("channels", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return Failure;
            }

            HelixChannel channel(data.toArray()[0].toObject());

            successCallback(channel);
            return Success;
        })
        .onError([failureCallback](auto /*result*/) {
            failureCallback();
        })
        .execute();
}

void Helix::createStreamMarker(
    QString broadcasterId, QString description,
    ResultCallback<HelixStreamMarker> successCallback,
    std::function<void(HelixStreamMarkerError)> failureCallback)
{
    QJsonObject payload;

    if (!description.isEmpty())
    {
        payload.insert("description", QJsonValue(description));
    }
    payload.insert("user_id", QJsonValue(broadcasterId));

    this->makeRequest("streams/markers", QUrlQuery())
        .type(NetworkRequestType::Post)
        .header("Content-Type", "application/json")
        .payload(QJsonDocument(payload).toJson(QJsonDocument::Compact))
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback(HelixStreamMarkerError::Unknown);
                return Failure;
            }

            HelixStreamMarker streamMarker(data.toArray()[0].toObject());

            successCallback(streamMarker);
            return Success;
        })
        .onError([failureCallback](NetworkResult result) {
            switch (result.status())
            {
                case 403: {
                    // User isn't a Channel Editor, so he can't create markers
                    failureCallback(HelixStreamMarkerError::UserNotAuthorized);
                }
                break;

                case 401: {
                    // User does not have the required scope to be able to create stream markers, user must reauthenticate
                    failureCallback(
                        HelixStreamMarkerError::UserNotAuthenticated);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Failed to create a stream marker: "
                        << result.status() << result.getData();
                    failureCallback(HelixStreamMarkerError::Unknown);
                }
                break;
            }
        })
        .execute();
};

void Helix::loadBlocks(QString userId,
                       ResultCallback<std::vector<HelixBlock>> successCallback,
                       HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("broadcaster_id", userId);
    urlQuery.addQueryItem("first", "100");

    this->makeRequest("users/blocks", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return Failure;
            }

            std::vector<HelixBlock> ignores;

            for (const auto &jsonStream : data.toArray())
            {
                ignores.emplace_back(jsonStream.toObject());
            }

            successCallback(ignores);

            return Success;
        })
        .onError([failureCallback](auto /*result*/) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

void Helix::blockUser(QString targetUserId,
                      std::function<void()> successCallback,
                      HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("target_user_id", targetUserId);

    this->makeRequest("users/blocks", urlQuery)
        .type(NetworkRequestType::Put)
        .onSuccess([successCallback](auto /*result*/) -> Outcome {
            successCallback();
            return Success;
        })
        .onError([failureCallback](auto /*result*/) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

void Helix::unblockUser(QString targetUserId,
                        std::function<void()> successCallback,
                        HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("target_user_id", targetUserId);

    this->makeRequest("users/blocks", urlQuery)
        .type(NetworkRequestType::Delete)
        .onSuccess([successCallback](auto /*result*/) -> Outcome {
            successCallback();
            return Success;
        })
        .onError([failureCallback](auto /*result*/) {
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

void Helix::manageAutoModMessages(
    QString userID, QString msgID, QString action,
    std::function<void()> successCallback,
    std::function<void(HelixAutoModMessageError)> failureCallback)
{
    QJsonObject payload;

    payload.insert("user_id", userID);
    payload.insert("msg_id", msgID);
    payload.insert("action", action);

    this->makeRequest("moderation/automod/message", QUrlQuery())
        .type(NetworkRequestType::Post)
        .header("Content-Type", "application/json")
        .payload(QJsonDocument(payload).toJson(QJsonDocument::Compact))
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            successCallback();
            return Success;
        })
        .onError([failureCallback, msgID, action](NetworkResult result) {
            switch (result.status())
            {
                case 400: {
                    // Message was already processed
                    failureCallback(
                        HelixAutoModMessageError::MessageAlreadyProcessed);
                }
                break;

                case 401: {
                    // User is missing the required scope
                    failureCallback(
                        HelixAutoModMessageError::UserNotAuthenticated);
                }
                break;

                case 403: {
                    // Requesting user is not authorized to manage messages
                    failureCallback(
                        HelixAutoModMessageError::UserNotAuthorized);
                }
                break;

                case 404: {
                    // Message not found or invalid msgID
                    failureCallback(HelixAutoModMessageError::MessageNotFound);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Failed to manage automod message: " << action
                        << msgID << result.status() << result.getData();
                    failureCallback(HelixAutoModMessageError::Unknown);
                }
                break;
            }
        })
        .execute();
}

void Helix::getCheermotes(
    QString broadcasterId,
    ResultCallback<std::vector<HelixCheermoteSet>> successCallback,
    HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;

    urlQuery.addQueryItem("broadcaster_id", broadcasterId);

    this->makeRequest("bits/cheermotes", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return Failure;
            }

            std::vector<HelixCheermoteSet> cheermoteSets;

            for (const auto &jsonStream : data.toArray())
            {
                cheermoteSets.emplace_back(jsonStream.toObject());
            }

            successCallback(cheermoteSets);
            return Success;
        })
        .onError([broadcasterId, failureCallback](NetworkResult result) {
            qCDebug(chatterinoTwitch)
                << "Failed to get cheermotes(broadcaster_id=" << broadcasterId
                << "): " << result.status() << result.getData();
            failureCallback();
        })
        .execute();
}

void Helix::getEmoteSetData(QString emoteSetId,
                            ResultCallback<HelixEmoteSetData> successCallback,
                            HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;

    urlQuery.addQueryItem("emote_set_id", emoteSetId);

    this->makeRequest("chat/emotes/set", urlQuery)
        .onSuccess([successCallback, failureCallback,
                    emoteSetId](auto result) -> Outcome {
            QJsonObject root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray() || data.toArray().isEmpty())
            {
                failureCallback();
                return Failure;
            }

            HelixEmoteSetData emoteSetData(data.toArray()[0].toObject());

            successCallback(emoteSetData);
            return Success;
        })
        .onError([failureCallback](NetworkResult result) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

void Helix::getChannelEmotes(
    QString broadcasterId,
    ResultCallback<std::vector<HelixChannelEmote>> successCallback,
    HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("broadcaster_id", broadcasterId);

    this->makeRequest("chat/emotes", urlQuery)
        .onSuccess([successCallback,
                    failureCallback](NetworkResult result) -> Outcome {
            QJsonObject root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return Failure;
            }

            std::vector<HelixChannelEmote> channelEmotes;

            for (const auto &jsonStream : data.toArray())
            {
                channelEmotes.emplace_back(jsonStream.toObject());
            }

            successCallback(channelEmotes);
            return Success;
        })
        .onError([failureCallback](auto result) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

void Helix::updateUserChatColor(
    QString userID, QString color, ResultCallback<> successCallback,
    FailureCallback<HelixUpdateUserChatColorError, QString> failureCallback)
{
    using Error = HelixUpdateUserChatColorError;

    QJsonObject payload;

    payload.insert("user_id", QJsonValue(userID));
    payload.insert("color", QJsonValue(color));

    this->makeRequest("chat/color", QUrlQuery())
        .type(NetworkRequestType::Put)
        .header("Content-Type", "application/json")
        .payload(QJsonDocument(payload).toJson(QJsonDocument::Compact))
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            auto obj = result.parseJson();
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for updating chat color was"
                    << result.status() << "but we only expected it to be 204";
            }

            successCallback();
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 400: {
                    if (message.startsWith("invalid color",
                                           Qt::CaseInsensitive))
                    {
                        // Handle this error specifically since it allows us to list out the available colors
                        failureCallback(Error::InvalidColor, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        // Handle this error specifically because its API error is especially unfriendly
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error changing user color:"
                        << result.status() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
};

void Helix::deleteChatMessages(
    QString broadcasterID, QString moderatorID, QString messageID,
    ResultCallback<> successCallback,
    FailureCallback<HelixDeleteChatMessagesError, QString> failureCallback)
{
    using Error = HelixDeleteChatMessagesError;

    QUrlQuery urlQuery;

    urlQuery.addQueryItem("broadcaster_id", broadcasterID);
    urlQuery.addQueryItem("moderator_id", moderatorID);

    if (!messageID.isEmpty())
    {
        // If message ID is empty, it's equivalent to /clear
        urlQuery.addQueryItem("message_id", messageID);
    }

    this->makeRequest("moderation/chat", urlQuery)
        .type(NetworkRequestType::Delete)
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for deleting chat messages was"
                    << result.status() << "but we only expected it to be 204";
            }

            successCallback();
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 404: {
                    // A 404 on this endpoint means message id is invalid or unable to be deleted.
                    // See: https://dev.twitch.tv/docs/api/reference#delete-chat-messages
                    failureCallback(Error::MessageUnavailable, message);
                }
                break;

                case 400: {
                    // These errors are generally well formatted, so we just forward them.
                    // This is currently undocumented behaviour, see: https://github.com/twitchdev/issues/issues/660
                    failureCallback(Error::Forwarded, message);
                }
                break;

                case 403: {
                    // 403 endpoint means the user does not have permission to perform this action in that channel
                    // Most likely to missing moderator permissions
                    // Missing documentation issue: https://github.com/twitchdev/issues/issues/659
                    // `message` value is well-formed so no need for a specific error type
                    failureCallback(Error::Forwarded, message);
                }
                break;

                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        // Handle this error specifically because its API error is especially unfriendly
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error deleting chat messages:"
                        << result.status() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

void Helix::addChannelModerator(
    QString broadcasterID, QString userID, ResultCallback<> successCallback,
    FailureCallback<HelixAddChannelModeratorError, QString> failureCallback)
{
    using Error = HelixAddChannelModeratorError;

    QUrlQuery urlQuery;

    urlQuery.addQueryItem("broadcaster_id", broadcasterID);
    urlQuery.addQueryItem("user_id", userID);

    this->makeRequest("moderation/moderators", urlQuery)
        .type(NetworkRequestType::Post)
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for adding a moderator was"
                    << result.status() << "but we only expected it to be 204";
            }

            successCallback();
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        // Handle this error specifically because its API error is especially unfriendly
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else if (message.compare("incorrect user authorization",
                                             Qt::CaseInsensitive) == 0)
                    {
                        // This error is pretty ugly, but essentially means they're not authorized to mod people in this channel
                        failureCallback(Error::UserNotAuthorized, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 400: {
                    if (message.compare("user is already a mod",
                                        Qt::CaseInsensitive) == 0)
                    {
                        // This error is particularly ugly, handle it separately
                        failureCallback(Error::TargetAlreadyModded, message);
                    }
                    else
                    {
                        // The Twitch API error sufficiently tells the user what went wrong
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 422: {
                    // Target is already a VIP
                    failureCallback(Error::TargetIsVIP, message);
                }
                break;

                case 429: {
                    // Endpoint has a strict ratelimit
                    failureCallback(Error::Ratelimited, message);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error adding channel moderator:"
                        << result.status() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

void Helix::removeChannelModerator(
    QString broadcasterID, QString userID, ResultCallback<> successCallback,
    FailureCallback<HelixRemoveChannelModeratorError, QString> failureCallback)
{
    using Error = HelixRemoveChannelModeratorError;

    QUrlQuery urlQuery;

    urlQuery.addQueryItem("broadcaster_id", broadcasterID);
    urlQuery.addQueryItem("user_id", userID);

    this->makeRequest("moderation/moderators", urlQuery)
        .type(NetworkRequestType::Delete)
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for unmodding user was"
                    << result.status() << "but we only expected it to be 204";
            }

            successCallback();
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 400: {
                    if (message.compare("user is not a mod",
                                        Qt::CaseInsensitive) == 0)
                    {
                        // This error message is particularly ugly, so we handle it differently
                        failureCallback(Error::TargetNotModded, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        // Handle this error specifically because its API error is especially unfriendly
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else if (message.compare("incorrect user authorization",
                                             Qt::CaseInsensitive) == 0)
                    {
                        failureCallback(Error::UserNotAuthorized, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 429: {
                    failureCallback(Error::Ratelimited, message);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error unmodding user:" << result.status()
                        << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

void Helix::sendChatAnnouncement(
    QString broadcasterID, QString moderatorID, QString message,
    HelixAnnouncementColor color, ResultCallback<> successCallback,
    FailureCallback<HelixSendChatAnnouncementError, QString> failureCallback)
{
    using Error = HelixSendChatAnnouncementError;

    QUrlQuery urlQuery;

    urlQuery.addQueryItem("broadcaster_id", broadcasterID);
    urlQuery.addQueryItem("moderator_id", moderatorID);

    QJsonObject body;
    body.insert("message", message);
    const auto colorStr =
        std::string{magic_enum::enum_name<HelixAnnouncementColor>(color)};
    body.insert("color", QString::fromStdString(colorStr).toLower());

    this->makeRequest("chat/announcements", urlQuery)
        .type(NetworkRequestType::Post)
        .header("Content-Type", "application/json")
        .payload(QJsonDocument(body).toJson(QJsonDocument::Compact))
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for sending an announcement was"
                    << result.status() << "but we only expected it to be 204";
            }

            successCallback();
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 400: {
                    // These errors are generally well formatted, so we just forward them.
                    // This is currently undocumented behaviour, see: https://github.com/twitchdev/issues/issues/660
                    failureCallback(Error::Forwarded, message);
                }
                break;

                case 403: {
                    // 403 endpoint means the user does not have permission to perform this action in that channel
                    // `message` value is well-formed so no need for a specific error type
                    failureCallback(Error::Forwarded, message);
                }
                break;

                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        // Handle this error specifically because its API error is especially unfriendly
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error sending an announcement:"
                        << result.status() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

void Helix::addChannelVIP(
    QString broadcasterID, QString userID, ResultCallback<> successCallback,
    FailureCallback<HelixAddChannelVIPError, QString> failureCallback)
{
    using Error = HelixAddChannelVIPError;

    QUrlQuery urlQuery;

    urlQuery.addQueryItem("broadcaster_id", broadcasterID);
    urlQuery.addQueryItem("user_id", userID);

    this->makeRequest("channels/vips", urlQuery)
        .type(NetworkRequestType::Post)
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for adding channel VIP was"
                    << result.status() << "but we only expected it to be 204";
            }

            successCallback();
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 400:
                case 409:
                case 422:
                case 425: {
                    // Most of the errors returned by this endpoint are pretty good. We can rely on Twitch's API messages
                    failureCallback(Error::Forwarded, message);
                }
                break;

                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        // Handle this error specifically because its API error is especially unfriendly
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else if (message.compare("incorrect user authorization",
                                             Qt::CaseInsensitive) == 0 ||
                             message.startsWith("the id in broadcaster_id must "
                                                "match the user id",
                                                Qt::CaseInsensitive))
                    {
                        // This error is particularly ugly, but is the equivalent to a user not having permissions
                        failureCallback(Error::UserNotAuthorized, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 429: {
                    failureCallback(Error::Ratelimited, message);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error adding channel VIP:"
                        << result.status() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

void Helix::removeChannelVIP(
    QString broadcasterID, QString userID, ResultCallback<> successCallback,
    FailureCallback<HelixRemoveChannelVIPError, QString> failureCallback)
{
    using Error = HelixRemoveChannelVIPError;

    QUrlQuery urlQuery;

    urlQuery.addQueryItem("broadcaster_id", broadcasterID);
    urlQuery.addQueryItem("user_id", userID);

    this->makeRequest("channels/vips", urlQuery)
        .type(NetworkRequestType::Delete)
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for removing channel VIP was"
                    << result.status() << "but we only expected it to be 204";
            }

            successCallback();
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 400:
                case 409:
                case 422: {
                    // Most of the errors returned by this endpoint are pretty good. We can rely on Twitch's API messages
                    failureCallback(Error::Forwarded, message);
                }
                break;

                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        // Handle this error specifically because its API error is especially unfriendly
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else if (message.compare("incorrect user authorization",
                                             Qt::CaseInsensitive) == 0 ||
                             message.startsWith("the id in broadcaster_id must "
                                                "match the user id",
                                                Qt::CaseInsensitive))
                    {
                        // This error is particularly ugly, but is the equivalent to a user not having permissions
                        failureCallback(Error::UserNotAuthorized, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 429: {
                    failureCallback(Error::Ratelimited, message);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error removing channel VIP:"
                        << result.status() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
void Helix::unbanUser(
    QString broadcasterID, QString moderatorID, QString userID,
    ResultCallback<> successCallback,
    FailureCallback<HelixUnbanUserError, QString> failureCallback)
{
    using Error = HelixUnbanUserError;

    QUrlQuery urlQuery;

    urlQuery.addQueryItem("broadcaster_id", broadcasterID);
    urlQuery.addQueryItem("moderator_id", moderatorID);
    urlQuery.addQueryItem("user_id", userID);

    this->makeRequest("moderation/bans", urlQuery)
        .type(NetworkRequestType::Delete)
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for unbanning user was"
                    << result.status() << "but we only expected it to be 204";
            }

            successCallback();
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 400: {
                    if (message.startsWith("The user in the user_id query "
                                           "parameter is not banned",
                                           Qt::CaseInsensitive))
                    {
                        failureCallback(Error::TargetNotBanned, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 409: {
                    failureCallback(Error::ConflictingOperation, message);
                }
                break;

                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        // Handle this error specifically because its API error is especially unfriendly
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else if (message.compare("incorrect user authorization",
                                             Qt::CaseInsensitive) == 0 ||
                             message.startsWith("the id in broadcaster_id must "
                                                "match the user id",
                                                Qt::CaseInsensitive))
                    {
                        // This error is particularly ugly, but is the equivalent to a user not having permissions
                        failureCallback(Error::UserNotAuthorized, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 403: {
                    failureCallback(Error::UserNotAuthorized, message);
                }
                break;

                case 429: {
                    failureCallback(Error::Ratelimited, message);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error unbanning user:" << result.status()
                        << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}  // These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch
// These changes are from the helix-command-migration/unban-untimeout branch

void Helix::startRaid(
    QString fromBroadcasterID, QString toBroadcasterID,
    ResultCallback<> successCallback,
    FailureCallback<HelixStartRaidError, QString> failureCallback)
{
    using Error = HelixStartRaidError;

    QUrlQuery urlQuery;

    urlQuery.addQueryItem("from_broadcaster_id", fromBroadcasterID);
    urlQuery.addQueryItem("to_broadcaster_id", toBroadcasterID);

    this->makeRequest("raids", urlQuery)
        .type(NetworkRequestType::Post)
        .onSuccess(
            [successCallback, failureCallback](auto /*result*/) -> Outcome {
                successCallback();
                return Success;
            })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 400: {
                    if (message.compare("The IDs in from_broadcaster_id and "
                                        "to_broadcaster_id cannot be the same.",
                                        Qt::CaseInsensitive) == 0)
                    {
                        failureCallback(Error::CantRaidYourself, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else if (message.compare(
                                 "The ID in broadcaster_id must match the user "
                                 "ID "
                                 "found in the request's OAuth token.",
                                 Qt::CaseInsensitive) == 0)
                    {
                        // Must be the broadcaster.
                        failureCallback(Error::UserNotAuthorized, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 409: {
                    failureCallback(Error::Forwarded, message);
                }
                break;

                case 429: {
                    failureCallback(Error::Ratelimited, message);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error while starting a raid:"
                        << result.status() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

void Helix::cancelRaid(
    QString broadcasterID, ResultCallback<> successCallback,
    FailureCallback<HelixCancelRaidError, QString> failureCallback)
{
    using Error = HelixCancelRaidError;

    QUrlQuery urlQuery;

    urlQuery.addQueryItem("broadcaster_id", broadcasterID);

    this->makeRequest("raids", urlQuery)
        .type(NetworkRequestType::Delete)
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for canceling the raid was"
                    << result.status() << "but we only expected it to be 204";
            }

            successCallback();
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else if (message.compare(
                                 "The ID in broadcaster_id must match the user "
                                 "ID "
                                 "found in the request's OAuth token.",
                                 Qt::CaseInsensitive) == 0)
                    {
                        // Must be the broadcaster.
                        failureCallback(Error::UserNotAuthorized, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 404: {
                    failureCallback(Error::NoRaidPending, message);
                }
                break;

                case 429: {
                    failureCallback(Error::Ratelimited, message);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error while canceling the raid:"
                        << result.status() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}  // cancelRaid

void Helix::updateEmoteMode(
    QString broadcasterID, QString moderatorID, bool emoteMode,
    ResultCallback<HelixChatSettings> successCallback,
    FailureCallback<HelixUpdateChatSettingsError, QString> failureCallback)
{
    QJsonObject json;
    json["emote_mode"] = emoteMode;
    this->updateChatSettings(broadcasterID, moderatorID, json, successCallback,
                             failureCallback);
}

void Helix::updateFollowerMode(
    QString broadcasterID, QString moderatorID,
    boost::optional<int> followerModeDuration,
    ResultCallback<HelixChatSettings> successCallback,
    FailureCallback<HelixUpdateChatSettingsError, QString> failureCallback)
{
    QJsonObject json;
    json["follower_mode"] = followerModeDuration.has_value();
    if (followerModeDuration)
    {
        json["follower_mode_duration"] = *followerModeDuration;
    }

    this->updateChatSettings(broadcasterID, moderatorID, json, successCallback,
                             failureCallback);
}

void Helix::updateNonModeratorChatDelay(
    QString broadcasterID, QString moderatorID,
    boost::optional<int> nonModeratorChatDelayDuration,
    ResultCallback<HelixChatSettings> successCallback,
    FailureCallback<HelixUpdateChatSettingsError, QString> failureCallback)
{
    QJsonObject json;
    json["non_moderator_chat_delay"] =
        nonModeratorChatDelayDuration.has_value();
    if (nonModeratorChatDelayDuration)
    {
        json["non_moderator_chat_delay_duration"] =
            *nonModeratorChatDelayDuration;
    }

    this->updateChatSettings(broadcasterID, moderatorID, json, successCallback,
                             failureCallback);
}

void Helix::updateSlowMode(
    QString broadcasterID, QString moderatorID,
    boost::optional<int> slowModeWaitTime,
    ResultCallback<HelixChatSettings> successCallback,
    FailureCallback<HelixUpdateChatSettingsError, QString> failureCallback)
{
    QJsonObject json;
    json["slow_mode"] = slowModeWaitTime.has_value();
    if (slowModeWaitTime)
    {
        json["slow_mode_wait_time"] = *slowModeWaitTime;
    }

    this->updateChatSettings(broadcasterID, moderatorID, json, successCallback,
                             failureCallback);
}

void Helix::updateSubscriberMode(
    QString broadcasterID, QString moderatorID, bool subscriberMode,
    ResultCallback<HelixChatSettings> successCallback,
    FailureCallback<HelixUpdateChatSettingsError, QString> failureCallback)
{
    QJsonObject json;
    json["subscriber_mode"] = subscriberMode;
    this->updateChatSettings(broadcasterID, moderatorID, json, successCallback,
                             failureCallback);
}

void Helix::updateUniqueChatMode(
    QString broadcasterID, QString moderatorID, bool uniqueChatMode,
    ResultCallback<HelixChatSettings> successCallback,
    FailureCallback<HelixUpdateChatSettingsError, QString> failureCallback)
{
    QJsonObject json;
    json["unique_chat_mode"] = uniqueChatMode;
    this->updateChatSettings(broadcasterID, moderatorID, json, successCallback,
                             failureCallback);
}

void Helix::updateChatSettings(
    QString broadcasterID, QString moderatorID, QJsonObject payload,
    ResultCallback<HelixChatSettings> successCallback,
    FailureCallback<HelixUpdateChatSettingsError, QString> failureCallback)
{
    using Error = HelixUpdateChatSettingsError;

    QUrlQuery urlQuery;

    urlQuery.addQueryItem("broadcaster_id", broadcasterID);
    urlQuery.addQueryItem("moderator_id", moderatorID);

    this->makeRequest("chat/settings", urlQuery)
        .type(NetworkRequestType::Patch)
        .header("Content-Type", "application/json")
        .payload(QJsonDocument(payload).toJson(QJsonDocument::Compact))
        .onSuccess([successCallback](auto result) -> Outcome {
            if (result.status() != 200)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for updating chat settings was"
                    << result.status() << "but we expected it to be 200";
            }
            auto response = result.parseJson();
            successCallback(HelixChatSettings(
                response.value("data").toArray().first().toObject()));
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 400: {
                    if (message.contains("must be in the range"))
                    {
                        failureCallback(Error::OutOfRange, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;
                case 409:
                case 422:
                case 425: {
                    failureCallback(Error::Forwarded, message);
                }
                break;

                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        // Handle this error specifically because its API error is especially unfriendly
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 403: {
                    failureCallback(Error::UserNotAuthorized, message);
                }
                break;

                case 429: {
                    failureCallback(Error::Ratelimited, message);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error updating chat settings:"
                        << result.status() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

void Helix::onFetchChattersSuccess(
    std::shared_ptr<HelixChatters> finalChatters, QString broadcasterID,
    QString moderatorID, int maxChattersToFetch,
    ResultCallback<HelixChatters> successCallback,
    FailureCallback<HelixGetChattersError, QString> failureCallback,
    HelixChatters chatters)
{
    qCDebug(chatterinoTwitch)
        << "Fetched" << chatters.chatters.size() << "chatters";

    finalChatters->chatters.merge(chatters.chatters);
    finalChatters->total = chatters.total;

    if (chatters.cursor.isEmpty() ||
        finalChatters->chatters.size() >= maxChattersToFetch)
    {
        // Done paginating
        successCallback(*finalChatters);
        return;
    }

    this->fetchChatters(
        broadcasterID, moderatorID, NUM_CHATTERS_TO_FETCH, chatters.cursor,
        [=](auto chatters) {
            this->onFetchChattersSuccess(
                finalChatters, broadcasterID, moderatorID, maxChattersToFetch,
                successCallback, failureCallback, chatters);
        },
        failureCallback);
}

// https://dev.twitch.tv/docs/api/reference#get-chatters
void Helix::fetchChatters(
    QString broadcasterID, QString moderatorID, int first, QString after,
    ResultCallback<HelixChatters> successCallback,
    FailureCallback<HelixGetChattersError, QString> failureCallback)
{
    using Error = HelixGetChattersError;

    QUrlQuery urlQuery;

    urlQuery.addQueryItem("broadcaster_id", broadcasterID);
    urlQuery.addQueryItem("moderator_id", moderatorID);
    urlQuery.addQueryItem("first", QString::number(first));

    if (!after.isEmpty())
    {
        urlQuery.addQueryItem("after", after);
    }

    this->makeRequest("chat/chatters", urlQuery)
        .onSuccess([successCallback](auto result) -> Outcome {
            if (result.status() != 200)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for getting chatters was "
                    << result.status() << "but we expected it to be 200";
            }

            auto response = result.parseJson();
            successCallback(HelixChatters(response));
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 400: {
                    failureCallback(Error::Forwarded, message);
                }
                break;

                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else if (message.contains("OAuth token"))
                    {
                        failureCallback(Error::UserNotAuthorized, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 403: {
                    failureCallback(Error::UserNotAuthorized, message);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error data:" << result.status()
                        << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

void Helix::onFetchModeratorsSuccess(
    std::shared_ptr<std::vector<HelixModerator>> finalModerators,
    QString broadcasterID, int maxModeratorsToFetch,
    ResultCallback<std::vector<HelixModerator>> successCallback,
    FailureCallback<HelixGetModeratorsError, QString> failureCallback,
    HelixModerators moderators)
{
    qCDebug(chatterinoTwitch)
        << "Fetched " << moderators.moderators.size() << " moderators";

    std::for_each(moderators.moderators.begin(), moderators.moderators.end(),
                  [finalModerators](auto mod) {
                      finalModerators->push_back(mod);
                  });

    if (moderators.cursor.isEmpty() ||
        finalModerators->size() >= maxModeratorsToFetch)
    {
        // Done paginating
        successCallback(*finalModerators);
        return;
    }

    this->fetchModerators(
        broadcasterID, NUM_MODERATORS_TO_FETCH_PER_REQUEST, moderators.cursor,
        [=](auto moderators) {
            this->onFetchModeratorsSuccess(
                finalModerators, broadcasterID, maxModeratorsToFetch,
                successCallback, failureCallback, moderators);
        },
        failureCallback);
}

// https://dev.twitch.tv/docs/api/reference#get-moderators
void Helix::fetchModerators(
    QString broadcasterID, int first, QString after,
    ResultCallback<HelixModerators> successCallback,
    FailureCallback<HelixGetModeratorsError, QString> failureCallback)
{
    using Error = HelixGetModeratorsError;

    QUrlQuery urlQuery;

    urlQuery.addQueryItem("broadcaster_id", broadcasterID);
    urlQuery.addQueryItem("first", QString::number(first));

    if (!after.isEmpty())
    {
        urlQuery.addQueryItem("after", after);
    }

    this->makeRequest("moderation/moderators", urlQuery)
        .onSuccess([successCallback](auto result) -> Outcome {
            if (result.status() != 200)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for getting moderators was "
                    << result.status() << "but we expected it to be 200";
            }

            auto response = result.parseJson();
            successCallback(HelixModerators(response));
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 400: {
                    failureCallback(Error::Forwarded, message);
                }
                break;

                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else if (message.contains("OAuth token"))
                    {
                        failureCallback(Error::UserNotAuthorized, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 403: {
                    failureCallback(Error::UserNotAuthorized, message);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error data:" << result.status()
                        << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

// Ban/timeout a user
// https://dev.twitch.tv/docs/api/reference#ban-user
void Helix::banUser(QString broadcasterID, QString moderatorID, QString userID,
                    boost::optional<int> duration, QString reason,
                    ResultCallback<> successCallback,
                    FailureCallback<HelixBanUserError, QString> failureCallback)
{
    using Error = HelixBanUserError;

    QUrlQuery urlQuery;

    urlQuery.addQueryItem("broadcaster_id", broadcasterID);
    urlQuery.addQueryItem("moderator_id", moderatorID);

    QJsonObject payload;
    {
        QJsonObject data;
        data["reason"] = reason;
        data["user_id"] = userID;
        if (duration)
        {
            data["duration"] = *duration;
        }

        payload["data"] = data;
    }

    this->makeRequest("moderation/bans", urlQuery)
        .type(NetworkRequestType::Post)
        .header("Content-Type", "application/json")
        .payload(QJsonDocument(payload).toJson(QJsonDocument::Compact))
        .onSuccess([successCallback](auto result) -> Outcome {
            if (result.status() != 200)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for banning a user was"
                    << result.status() << "but we expected it to be 200";
            }
            // we don't care about the response
            successCallback();
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 400: {
                    if (message.startsWith("The user specified in the user_id "
                                           "field is already banned",
                                           Qt::CaseInsensitive))
                    {
                        failureCallback(Error::TargetBanned, message);
                    }
                    else if (message.startsWith(
                                 "The user specified in the user_id field may "
                                 "not be banned",
                                 Qt::CaseInsensitive))
                    {
                        failureCallback(Error::CannotBanUser, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 409: {
                    failureCallback(Error::ConflictingOperation, message);
                }
                break;

                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        // Handle this error specifically because its API error is especially unfriendly
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 403: {
                    failureCallback(Error::UserNotAuthorized, message);
                }
                break;

                case 429: {
                    failureCallback(Error::Ratelimited, message);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error banning user:" << result.status()
                        << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

// https://dev.twitch.tv/docs/api/reference#send-whisper
void Helix::sendWhisper(
    QString fromUserID, QString toUserID, QString message,
    ResultCallback<> successCallback,
    FailureCallback<HelixWhisperError, QString> failureCallback)
{
    using Error = HelixWhisperError;

    QUrlQuery urlQuery;

    urlQuery.addQueryItem("from_user_id", fromUserID);
    urlQuery.addQueryItem("to_user_id", toUserID);

    QJsonObject payload;
    payload["message"] = message;

    this->makeRequest("whispers", urlQuery)
        .type(NetworkRequestType::Post)
        .header("Content-Type", "application/json")
        .payload(QJsonDocument(payload).toJson(QJsonDocument::Compact))
        .onSuccess([successCallback](auto result) -> Outcome {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for sending a whisper was"
                    << result.status() << "but we expected it to be 204";
            }
            // we don't care about the response
            successCallback();
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 400: {
                    if (message.startsWith("A user cannot whisper themself",
                                           Qt::CaseInsensitive))
                    {
                        failureCallback(Error::WhisperSelf, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        // Handle this error specifically because its API error is especially unfriendly
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else if (message.startsWith("the sender does not have a "
                                                "verified phone number",
                                                Qt::CaseInsensitive))
                    {
                        failureCallback(Error::NoVerifiedPhone, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 403: {
                    if (message.startsWith("The recipient's settings prevent "
                                           "this sender from whispering them",
                                           Qt::CaseInsensitive))
                    {
                        failureCallback(Error::RecipientBlockedUser, message);
                    }
                    else
                    {
                        failureCallback(Error::UserNotAuthorized, message);
                    }
                }
                break;

                case 404: {
                    failureCallback(Error::Forwarded, message);
                }
                break;

                case 429: {
                    failureCallback(Error::Ratelimited, message);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error banning user:" << result.status()
                        << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

// https://dev.twitch.tv/docs/api/reference#get-chatters
void Helix::getChatters(
    QString broadcasterID, QString moderatorID, int maxChattersToFetch,
    ResultCallback<HelixChatters> successCallback,
    FailureCallback<HelixGetChattersError, QString> failureCallback)
{
    auto finalChatters = std::make_shared<HelixChatters>();

    // Initiate the recursive calls
    this->fetchChatters(
        broadcasterID, moderatorID, NUM_CHATTERS_TO_FETCH, "",
        [=](auto chatters) {
            this->onFetchChattersSuccess(
                finalChatters, broadcasterID, moderatorID, maxChattersToFetch,
                successCallback, failureCallback, chatters);
        },
        failureCallback);
}

// https://dev.twitch.tv/docs/api/reference#get-moderators
void Helix::getModerators(
    QString broadcasterID, int maxModeratorsToFetch,
    ResultCallback<std::vector<HelixModerator>> successCallback,
    FailureCallback<HelixGetModeratorsError, QString> failureCallback)
{
    auto finalModerators = std::make_shared<std::vector<HelixModerator>>();

    // Initiate the recursive calls
    this->fetchModerators(
        broadcasterID, NUM_MODERATORS_TO_FETCH_PER_REQUEST, "",
        [=](auto moderators) {
            this->onFetchModeratorsSuccess(
                finalModerators, broadcasterID, maxModeratorsToFetch,
                successCallback, failureCallback, moderators);
        },
        failureCallback);
}

// List the VIPs of a channel
// https://dev.twitch.tv/docs/api/reference#get-vips
void Helix::getChannelVIPs(
    QString broadcasterID,
    ResultCallback<std::vector<HelixVip>> successCallback,
    FailureCallback<HelixListVIPsError, QString> failureCallback)
{
    using Error = HelixListVIPsError;
    QUrlQuery urlQuery;

    urlQuery.addQueryItem("broadcaster_id", broadcasterID);

    // No point pagi/pajanating, Twitch's max VIP count doesn't go over 100
    // TODO(jammehcow): probably still implement pagination
    //   as the mod list can go over 100 (I assume, I see no limit)
    urlQuery.addQueryItem("first", "100");

    this->makeRequest("channels/vips", urlQuery)
        .type(NetworkRequestType::Get)
        .header("Content-Type", "application/json")
        .onSuccess([successCallback](auto result) -> Outcome {
            if (result.status() != 200)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for getting VIPs was" << result.status()
                    << "but we expected it to be 200";
            }

            auto response = result.parseJson();

            std::vector<HelixVip> channelVips;
            for (const auto &jsonStream : response.value("data").toArray())
            {
                channelVips.emplace_back(jsonStream.toObject());
            }

            successCallback(channelVips);
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 400: {
                    failureCallback(Error::Forwarded, message);
                }
                break;

                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else if (message.compare(
                                 "The ID in broadcaster_id must match the user "
                                 "ID found in the request's OAuth token.",
                                 Qt::CaseInsensitive) == 0)
                    {
                        // Must be the broadcaster.
                        failureCallback(Error::UserNotBroadcaster, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 403: {
                    failureCallback(Error::UserNotAuthorized, message);
                }
                break;

                case 429: {
                    failureCallback(Error::Ratelimited, message);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error listing VIPs:" << result.status()
                        << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

void Helix::startCommercial(
    QString broadcasterID, int length,
    ResultCallback<HelixStartCommercialResponse> successCallback,
    FailureCallback<HelixStartCommercialError, QString> failureCallback)
{
    using Error = HelixStartCommercialError;

    QJsonObject payload;

    payload.insert("broadcaster_id", QJsonValue(broadcasterID));
    payload.insert("length", QJsonValue(length));

    this->makeRequest("channels/commercial", QUrlQuery())
        .type(NetworkRequestType::Post)
        .header("Content-Type", "application/json")
        .payload(QJsonDocument(payload).toJson(QJsonDocument::Compact))
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            auto obj = result.parseJson();
            if (obj.isEmpty())
            {
                failureCallback(
                    Error::Unknown,
                    "Twitch didn't send any information about this error.");
                return Failure;
            }

            successCallback(HelixStartCommercialResponse(obj));
            return Success;
        })
        .onError([failureCallback](auto result) {
            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (result.status())
            {
                case 400: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else if (message.contains(
                                 "To start a commercial, the broadcaster must "
                                 "be streaming live.",
                                 Qt::CaseInsensitive))
                    {
                        failureCallback(Error::BroadcasterNotStreaming,
                                        message);
                    }
                    else if (message.startsWith("Missing required parameter",
                                                Qt::CaseInsensitive))
                    {
                        failureCallback(Error::MissingLengthParameter, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 401: {
                    if (message.contains(
                            "The ID in broadcaster_id must match the user ID "
                            "found in the request's OAuth token.",
                            Qt::CaseInsensitive))
                    {
                        failureCallback(Error::TokenMustMatchBroadcaster,
                                        message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 429: {
                    // The cooldown period is implied to be included
                    // in the error's "retry_after" response field but isn't.
                    // If this becomes available we should append that to the error message.
                    failureCallback(Error::Ratelimited, message);
                }
                break;

                default: {
                    qCDebug(chatterinoTwitch)
                        << "Unhandled error starting commercial:"
                        << result.status() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
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
    this->clientId = std::move(clientId);
    this->oauthToken = std::move(oauthToken);
}

void Helix::initialize()
{
    assert(instance == nullptr);

    initializeHelix(new Helix());
}

void initializeHelix(IHelix *_instance)
{
    assert(_instance != nullptr);

    instance = _instance;
}

IHelix *getHelix()
{
    assert(instance != nullptr);

    return instance;
}

}  // namespace chatterino
