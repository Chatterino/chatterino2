#include "providers/twitch/api/Helix.hpp"

#include "common/Outcome.hpp"
#include "common/QLogging.hpp"

#include <QJsonDocument>
#include <magic_enum.hpp>

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
