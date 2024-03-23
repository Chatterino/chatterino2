#include "providers/twitch/api/Helix.hpp"

#include "common/Literals.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "util/CancellationToken.hpp"
#include "util/QMagicEnum.hpp"

#include <magic_enum/magic_enum.hpp>
#include <QJsonDocument>

namespace {

using namespace chatterino;

constexpr auto NUM_MODERATORS_TO_FETCH_PER_REQUEST = 100;

constexpr auto NUM_CHATTERS_TO_FETCH = 1000;

}  // namespace

namespace chatterino {

using namespace literals;

static IHelix *instance = nullptr;

HelixChatters::HelixChatters(const QJsonObject &jsonObject)
    : total(jsonObject.value("total").toInt())
    , cursor(
          jsonObject.value("pagination").toObject().value("cursor").toString())
{
    const auto &data = jsonObject.value("data").toArray();
    for (const auto &chatter : data)
    {
        auto userLogin = chatter.toObject().value("user_login").toString();
        this->chatters.insert(userLogin);
    }
}

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
    this->makeGet("users", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return;
            }

            std::vector<HelixUser> users;

            for (const auto &jsonUser : data.toArray())
            {
                users.emplace_back(jsonUser.toObject());
            }

            successCallback(users);
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

void Helix::getChannelFollowers(
    QString broadcasterID,
    ResultCallback<HelixGetChannelFollowersResponse> successCallback,
    std::function<void(QString)> failureCallback)
{
    assert(!broadcasterID.isEmpty());

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("broadcaster_id", broadcasterID);

    // TODO: set on success and on error
    this->makeGet("channels/followers", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) {
            auto root = result.parseJson();
            if (root.empty())
            {
                failureCallback("Bad JSON response");
                return;
            }
            successCallback(HelixGetChannelFollowersResponse(root));
        })
        .onError([failureCallback](auto result) {
            auto root = result.parseJson();
            if (root.empty())
            {
                failureCallback("Unknown error");
                return;
            }

            // Forward "message" from Twitch
            HelixError error(root);
            failureCallback(error.message);
        })
        .execute();
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
    this->makeGet("streams", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return;
            }

            std::vector<HelixStream> streams;

            for (const auto &jsonStream : data.toArray())
            {
                streams.emplace_back(jsonStream.toObject());
            }

            successCallback(streams);
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
    this->makeGet("games", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return;
            }

            std::vector<HelixGame> games;

            for (const auto &jsonStream : data.toArray())
            {
                games.emplace_back(jsonStream.toObject());
            }

            successCallback(games);
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

    this->makeGet("search/categories", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return;
            }

            std::vector<HelixGame> games;

            for (const auto &jsonStream : data.toArray())
            {
                games.emplace_back(jsonStream.toObject());
            }

            successCallback(games);
        })
        .onError([failureCallback](auto /*result*/) {
            // TODO: make better xd
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

    this->makePost("clips", urlQuery)
        .header("Content-Type", "application/json")
        .onSuccess([successCallback, failureCallback](auto result) {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback(HelixClipError::Unknown);
                return;
            }

            HelixClip clip(data.toArray()[0].toObject());

            successCallback(clip);
        })
        .onError([failureCallback](auto result) {
            switch (result.status().value_or(0))
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
                        << "Failed to create a clip: " << result.formatError()
                        << result.getData();
                    failureCallback(HelixClipError::Unknown);
                }
                break;
            }
        })
        .finally(std::move(finallyCallback))
        .execute();
}

void Helix::fetchChannels(
    QStringList userIDs,
    ResultCallback<std::vector<HelixChannel>> successCallback,
    HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;

    for (const auto &userID : userIDs)
    {
        urlQuery.addQueryItem("broadcaster_id", userID);
    }

    this->makeGet("channels", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return;
            }

            std::vector<HelixChannel> channels;

            for (const auto &unparsedChannel : data.toArray())
            {
                channels.emplace_back(unparsedChannel.toObject());
            }

            successCallback(channels);
        })
        .onError([failureCallback](auto /*result*/) {
            failureCallback();
        })
        .execute();
}

void Helix::getChannel(QString broadcasterId,
                       ResultCallback<HelixChannel> successCallback,
                       HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("broadcaster_id", broadcasterId);

    this->makeGet("channels", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return;
            }

            HelixChannel channel(data.toArray()[0].toObject());

            successCallback(channel);
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

    this->makePost("streams/markers", QUrlQuery())
        .json(payload)
        .onSuccess([successCallback, failureCallback](auto result) {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback(HelixStreamMarkerError::Unknown);
                return;
            }

            HelixStreamMarker streamMarker(data.toArray()[0].toObject());

            successCallback(streamMarker);
        })
        .onError([failureCallback](NetworkResult result) {
            switch (result.status().value_or(0))
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
                        << result.formatError() << result.getData();
                    failureCallback(HelixStreamMarkerError::Unknown);
                }
                break;
            }
        })
        .execute();
};

void Helix::loadBlocks(QString userId,
                       ResultCallback<std::vector<HelixBlock>> pageCallback,
                       FailureCallback<QString> failureCallback,
                       CancellationToken &&token)
{
    constexpr const size_t blockLimit = 1000;

    // TODO(Qt 5.13): use initializer list
    QUrlQuery query;
    query.addQueryItem(u"broadcaster_id"_s, userId);
    query.addQueryItem(u"first"_s, u"100"_s);

    size_t receivedItems = 0;
    this->paginate(
        u"users/blocks"_s, query,
        [pageCallback, receivedItems](const QJsonObject &json) mutable {
            const auto data = json["data"_L1].toArray();

            if (data.isEmpty())
            {
                return false;
            }

            std::vector<HelixBlock> ignores;
            ignores.reserve(data.count());

            for (const auto &ignore : data)
            {
                ignores.emplace_back(ignore.toObject());
            }

            pageCallback(ignores);

            receivedItems += data.count();

            if (receivedItems >= blockLimit)
            {
                qCInfo(chatterinoTwitch) << "Reached the limit of" << blockLimit
                                         << "Twitch blocks fetched";
                return false;
            }

            return true;
        },
        [failureCallback](const NetworkResult &result) {
            failureCallback(result.formatError());
        },
        std::move(token));
}

void Helix::blockUser(QString targetUserId, const QObject *caller,
                      std::function<void()> successCallback,
                      HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("target_user_id", targetUserId);

    this->makePut("users/blocks", urlQuery)
        .caller(caller)
        .onSuccess([successCallback](auto /*result*/) {
            successCallback();
        })
        .onError([failureCallback](auto /*result*/) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

void Helix::unblockUser(QString targetUserId, const QObject *caller,
                        std::function<void()> successCallback,
                        HelixFailureCallback failureCallback)
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("target_user_id", targetUserId);

    this->makeDelete("users/blocks", urlQuery)
        .caller(caller)
        .onSuccess([successCallback](auto /*result*/) {
            successCallback();
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

    urlQuery.addQueryItem("broadcaster_id", broadcasterId);
    this->makePatch("channels", urlQuery)
        .json(obj)
        .onSuccess([successCallback, failureCallback](auto result) {
            successCallback(result);
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

    this->makePost("moderation/automod/message", QUrlQuery())
        .json(payload)
        .onSuccess([successCallback, failureCallback](auto result) {
            successCallback();
        })
        .onError([failureCallback, msgID, action](NetworkResult result) {
            switch (result.status().value_or(0))
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
                        << msgID << result.formatError() << result.getData();
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

    this->makeGet("bits/cheermotes", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) {
            auto root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return;
            }

            std::vector<HelixCheermoteSet> cheermoteSets;

            for (const auto &jsonStream : data.toArray())
            {
                cheermoteSets.emplace_back(jsonStream.toObject());
            }

            successCallback(cheermoteSets);
        })
        .onError([broadcasterId, failureCallback](NetworkResult result) {
            qCDebug(chatterinoTwitch)
                << "Failed to get cheermotes(broadcaster_id=" << broadcasterId
                << "): " << result.formatError() << result.getData();
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

    this->makeGet("chat/emotes/set", urlQuery)
        .onSuccess([successCallback, failureCallback, emoteSetId](auto result) {
            QJsonObject root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray() || data.toArray().isEmpty())
            {
                failureCallback();
                return;
            }

            HelixEmoteSetData emoteSetData(data.toArray()[0].toObject());

            successCallback(emoteSetData);
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

    this->makeGet("chat/emotes", urlQuery)
        .onSuccess([successCallback, failureCallback](NetworkResult result) {
            QJsonObject root = result.parseJson();
            auto data = root.value("data");

            if (!data.isArray())
            {
                failureCallback();
                return;
            }

            std::vector<HelixChannelEmote> channelEmotes;

            for (const auto &jsonStream : data.toArray())
            {
                channelEmotes.emplace_back(jsonStream.toObject());
            }

            successCallback(channelEmotes);
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

    this->makePut("chat/color", QUrlQuery())
        .json(payload)
        .onSuccess([successCallback, failureCallback](auto result) {
            auto obj = result.parseJson();
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for updating chat color was"
                    << result.formatError()
                    << "but we only expected it to be 204";
            }

            successCallback();
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << result.formatError() << result.getData() << obj;
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

    this->makeDelete("moderation/chat", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for deleting chat messages was"
                    << result.formatError()
                    << "but we only expected it to be 204";
            }

            successCallback();
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << result.formatError() << result.getData() << obj;
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

    this->makePost("moderation/moderators", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for adding a moderator was"
                    << result.formatError()
                    << "but we only expected it to be 204";
            }

            successCallback();
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << result.formatError() << result.getData() << obj;
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

    this->makeDelete("moderation/moderators", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for unmodding user was"
                    << result.formatError()
                    << "but we only expected it to be 204";
            }

            successCallback();
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << "Unhandled error unmodding user:"
                        << result.formatError() << result.getData() << obj;
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
    body.insert("color", qmagicenum::enumNameString(color).toLower());

    this->makePost("chat/announcements", urlQuery)
        .json(body)
        .onSuccess([successCallback, failureCallback](auto result) {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for sending an announcement was"
                    << result.formatError()
                    << "but we only expected it to be 204";
            }

            successCallback();
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << result.formatError() << result.getData() << obj;
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

    this->makePost("channels/vips", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for adding channel VIP was"
                    << result.formatError()
                    << "but we only expected it to be 204";
            }

            successCallback();
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << result.formatError() << result.getData() << obj;
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

    this->makeDelete("channels/vips", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for removing channel VIP was"
                    << result.formatError()
                    << "but we only expected it to be 204";
            }

            successCallback();
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << result.formatError() << result.getData() << obj;
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

    this->makeDelete("moderation/bans", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for unbanning user was"
                    << result.formatError()
                    << "but we only expected it to be 204";
            }

            successCallback();
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << "Unhandled error unbanning user:"
                        << result.formatError() << result.getData() << obj;
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

    this->makePost("raids", urlQuery)
        .onSuccess([successCallback, failureCallback](auto /*result*/) {
            successCallback();
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << result.formatError() << result.getData() << obj;
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

    this->makeDelete("raids", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for canceling the raid was"
                    << result.formatError()
                    << "but we only expected it to be 204";
            }

            successCallback();
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << result.formatError() << result.getData() << obj;
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
    std::optional<int> followerModeDuration,
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
    std::optional<int> nonModeratorChatDelayDuration,
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
    std::optional<int> slowModeWaitTime,
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

    this->makePatch("chat/settings", urlQuery)
        .json(payload)
        .onSuccess([successCallback](auto result) {
            if (result.status() != 200)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for updating chat settings was"
                    << result.formatError() << "but we expected it to be 200";
            }
            auto response = result.parseJson();
            successCallback(HelixChatSettings(
                response.value("data").toArray().first().toObject()));
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << result.formatError() << result.getData() << obj;
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
        [=, this](auto chatters) {
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

    this->makeGet("chat/chatters", urlQuery)
        .onSuccess([successCallback](auto result) {
            if (result.status() != 200)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for getting chatters was "
                    << result.formatError() << "but we expected it to be 200";
            }

            auto response = result.parseJson();
            successCallback(HelixChatters(response));
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << "Unhandled error data:" << result.formatError()
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
        [=, this](auto moderators) {
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

    this->makeGet("moderation/moderators", urlQuery)
        .onSuccess([successCallback](auto result) {
            if (result.status() != 200)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for getting moderators was "
                    << result.formatError() << "but we expected it to be 200";
            }

            auto response = result.parseJson();
            successCallback(HelixModerators(response));
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << "Unhandled error data:" << result.formatError()
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
                    std::optional<int> duration, QString reason,
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

    this->makePost("moderation/bans", urlQuery)
        .json(payload)
        .onSuccess([successCallback](auto result) {
            if (result.status() != 200)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for banning a user was"
                    << result.formatError() << "but we expected it to be 200";
            }
            // we don't care about the response
            successCallback();
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << "Unhandled error banning user:"
                        << result.formatError() << result.getData() << obj;
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

    this->makePost("whispers", urlQuery)
        .json(payload)
        .onSuccess([successCallback](auto result) {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for sending a whisper was"
                    << result.formatError() << "but we expected it to be 204";
            }
            // we don't care about the response
            successCallback();
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << "Unhandled error banning user:"
                        << result.formatError() << result.getData() << obj;
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
        [=, this](auto chatters) {
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
        [=, this](auto moderators) {
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

    this->makeGet("channels/vips", urlQuery)
        .header("Content-Type", "application/json")
        .onSuccess([successCallback](auto result) {
            if (result.status() != 200)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for getting VIPs was"
                    << result.formatError() << "but we expected it to be 200";
            }

            auto response = result.parseJson();

            std::vector<HelixVip> channelVips;
            for (const auto &jsonStream : response.value("data").toArray())
            {
                channelVips.emplace_back(jsonStream.toObject());
            }

            successCallback(channelVips);
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << "Unhandled error listing VIPs:"
                        << result.formatError() << result.getData() << obj;
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

    this->makePost("channels/commercial", QUrlQuery())
        .json(payload)
        .onSuccess([successCallback, failureCallback](auto result) {
            auto obj = result.parseJson();
            if (obj.isEmpty())
            {
                failureCallback(
                    Error::Unknown,
                    "Twitch didn't send any information about this error.");
                return;
            }

            successCallback(HelixStartCommercialResponse(obj));
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
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
                        << result.formatError() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

// Twitch global badges
// https://dev.twitch.tv/docs/api/reference/#get-global-chat-badges
void Helix::getGlobalBadges(
    ResultCallback<HelixGlobalBadges> successCallback,
    FailureCallback<HelixGetGlobalBadgesError, QString> failureCallback)
{
    using Error = HelixGetGlobalBadgesError;

    this->makeGet("chat/badges/global", QUrlQuery())
        .onSuccess([successCallback](auto result) {
            if (result.status() != 200)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for getting global badges was "
                    << result.formatError() << "but we expected it to be 200";
            }

            auto response = result.parseJson();
            successCallback(HelixGlobalBadges(response));
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
            {
                case 401: {
                    failureCallback(Error::Forwarded, message);
                }
                break;

                default: {
                    qCWarning(chatterinoTwitch)
                        << "Helix global badges, unhandled error data:"
                        << result.formatError() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

// Badges for the `broadcasterID` channel
// https://dev.twitch.tv/docs/api/reference/#get-channel-chat-badges
void Helix::getChannelBadges(
    QString broadcasterID, ResultCallback<HelixChannelBadges> successCallback,
    FailureCallback<HelixGetChannelBadgesError, QString> failureCallback)
{
    using Error = HelixGetChannelBadgesError;

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("broadcaster_id", broadcasterID);

    this->makeGet("chat/badges", urlQuery)
        .onSuccess([successCallback](auto result) {
            if (result.status() != 200)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for getting badges was "
                    << result.formatError() << "but we expected it to be 200";
            }

            auto response = result.parseJson();
            successCallback(HelixChannelBadges(response));
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            auto obj = result.parseJson();
            auto message = obj.value("message").toString();

            switch (*result.status())
            {
                case 400:
                case 401: {
                    failureCallback(Error::Forwarded, message);
                }
                break;

                default: {
                    qCWarning(chatterinoTwitch)
                        << "Helix channel badges, unhandled error data:"
                        << result.formatError() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

// https://dev.twitch.tv/docs/api/reference/#update-shield-mode-status
void Helix::updateShieldMode(
    QString broadcasterID, QString moderatorID, bool isActive,
    ResultCallback<HelixShieldModeStatus> successCallback,
    FailureCallback<HelixUpdateShieldModeError, QString> failureCallback)
{
    using Error = HelixUpdateShieldModeError;

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("broadcaster_id", broadcasterID);
    urlQuery.addQueryItem("moderator_id", moderatorID);

    QJsonObject payload;
    payload["is_active"] = isActive;

    this->makePut("moderation/shield_mode", urlQuery)
        .json(payload)
        .onSuccess([successCallback](auto result) {
            if (result.status() != 200)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for updating shield mode was "
                    << result.formatError() << "but we expected it to be 200";
            }

            const auto response = result.parseJson();
            successCallback(
                HelixShieldModeStatus(response["data"][0].toObject()));
        })
        .onError([failureCallback](const auto &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            const auto obj = result.parseJson();
            auto message = obj["message"].toString();

            switch (*result.status())
            {
                case 400: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        failureCallback(Error::UserMissingScope, message);
                        break;
                    }

                    failureCallback(Error::Forwarded, message);
                }
                break;
                case 401: {
                    failureCallback(Error::Forwarded, message);
                }
                break;
                case 403: {
                    if (message.startsWith(
                            "Requester does not have permissions",
                            Qt::CaseInsensitive))
                    {
                        failureCallback(Error::MissingPermission, message);
                        break;
                    }
                }

                default: {
                    qCWarning(chatterinoTwitch)
                        << "Helix shield mode, unhandled error data:"
                        << result.formatError() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
                break;
            }
        })
        .execute();
}

// https://dev.twitch.tv/docs/api/reference/#send-a-shoutout
void Helix::sendShoutout(
    QString fromBroadcasterID, QString toBroadcasterID, QString moderatorID,
    ResultCallback<> successCallback,
    FailureCallback<HelixSendShoutoutError, QString> failureCallback)
{
    using Error = HelixSendShoutoutError;

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("from_broadcaster_id", fromBroadcasterID);
    urlQuery.addQueryItem("to_broadcaster_id", toBroadcasterID);
    urlQuery.addQueryItem("moderator_id", moderatorID);

    this->makePost("chat/shoutouts", urlQuery)
        .header("Content-Type", "application/json")
        .onSuccess([successCallback](NetworkResult result) {
            if (result.status() != 204)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for sending shoutout was "
                    << result.formatError() << "but we expected it to be 204";
            }

            successCallback();
        })
        .onError([failureCallback](const NetworkResult &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            const auto obj = result.parseJson();
            auto message = obj["message"].toString();

            switch (*result.status())
            {
                case 400: {
                    if (message.startsWith("The broadcaster may not give "
                                           "themselves a Shoutout.",
                                           Qt::CaseInsensitive))
                    {
                        failureCallback(Error::UserIsBroadcaster, message);
                    }
                    else if (message.startsWith(
                                 "The broadcaster is not streaming live or "
                                 "does not have one or more viewers.",
                                 Qt::CaseInsensitive))
                    {
                        failureCallback(Error::BroadcasterNotLive, message);
                    }
                    else
                    {
                        failureCallback(Error::UserNotAuthorized, message);
                    }
                }
                break;

                case 401: {
                    if (message.startsWith("Missing scope",
                                           Qt::CaseInsensitive))
                    {
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else
                    {
                        failureCallback(Error::UserNotAuthorized, message);
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

                case 500: {
                    if (message.isEmpty())
                    {
                        failureCallback(Error::Unknown,
                                        "Twitch internal server error");
                    }
                    else
                    {
                        failureCallback(Error::Unknown, message);
                    }
                }
                break;

                default: {
                    qCWarning(chatterinoTwitch)
                        << "Helix send shoutout, unhandled error data:"
                        << result.formatError() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
            }
        })
        .execute();
}

// https://dev.twitch.tv/docs/api/reference/#send-chat-message
void Helix::sendChatMessage(
    HelixSendMessageArgs args, ResultCallback<HelixSentMessage> successCallback,
    FailureCallback<HelixSendMessageError, QString> failureCallback)
{
    using Error = HelixSendMessageError;

    QJsonObject json{{
        {"broadcaster_id", args.broadcasterID},
        {"sender_id", args.senderID},
        {"message", args.message},
    }};
    if (!args.replyParentMessageID.isEmpty())
    {
        json["reply_parent_message_id"] = args.replyParentMessageID;
    }

    this->makePost("chat/messages", {})
        .json(json)
        .onSuccess([successCallback](const NetworkResult &result) {
            if (result.status() != 200)
            {
                qCWarning(chatterinoTwitch)
                    << "Success result for sending chat message was "
                    << result.formatError() << "but we expected it to be 200";
            }
            auto json = result.parseJson();

            successCallback(HelixSentMessage(
                json.value("data").toArray().at(0).toObject()));
        })
        .onError([failureCallback](const NetworkResult &result) -> void {
            if (!result.status())
            {
                failureCallback(Error::Unknown, result.formatError());
                return;
            }

            const auto obj = result.parseJson();
            auto message =
                obj["message"].toString(u"Twitch internal server error"_s);

            switch (*result.status())
            {
                case 400: {
                    failureCallback(Error::Unknown, message);
                }
                break;

                case 401: {
                    if (message.startsWith("User access token requires the",
                                           Qt::CaseInsensitive))
                    {
                        failureCallback(Error::UserMissingScope, message);
                    }
                    else
                    {
                        failureCallback(Error::Forwarded, message);
                    }
                }
                break;

                case 403: {
                    failureCallback(Error::Forbidden, message);
                }
                break;

                case 422: {
                    failureCallback(Error::MessageTooLarge, message);
                }
                break;

                case 500: {
                    failureCallback(Error::Unknown, message);
                }
                break;

                default: {
                    qCWarning(chatterinoTwitch)
                        << "Helix send chat message, unhandled error data:"
                        << result.formatError() << result.getData() << obj;
                    failureCallback(Error::Unknown, message);
                }
            }
        })
        .execute();
}

NetworkRequest Helix::makeRequest(const QString &url, const QUrlQuery &urlQuery,
                                  NetworkRequestType type)
{
    assert(!url.startsWith("/"));

    if (this->clientId.isEmpty())
    {
        qCDebug(chatterinoTwitch)
            << "Helix::makeRequest called without a client ID set BabyRage";
        // return std::nullopt;
    }

    if (this->oauthToken.isEmpty())
    {
        qCDebug(chatterinoTwitch)
            << "Helix::makeRequest called without an oauth token set BabyRage";
        // return std::nullopt;
    }

    const QString baseUrl("https://api.twitch.tv/helix/");

    QUrl fullUrl(baseUrl + url);

    fullUrl.setQuery(urlQuery);

    return NetworkRequest(fullUrl, type)
        .timeout(5 * 1000)
        .header("Accept", "application/json")
        .header("Client-ID", this->clientId)
        .header("Authorization", "Bearer " + this->oauthToken);
}

NetworkRequest Helix::makeGet(const QString &url, const QUrlQuery &urlQuery)
{
    return this->makeRequest(url, urlQuery, NetworkRequestType::Get);
}

NetworkRequest Helix::makeDelete(const QString &url, const QUrlQuery &urlQuery)
{
    return this->makeRequest(url, urlQuery, NetworkRequestType::Delete);
}

NetworkRequest Helix::makePost(const QString &url, const QUrlQuery &urlQuery)
{
    return this->makeRequest(url, urlQuery, NetworkRequestType::Post);
}

NetworkRequest Helix::makePut(const QString &url, const QUrlQuery &urlQuery)
{
    return this->makeRequest(url, urlQuery, NetworkRequestType::Put);
}

NetworkRequest Helix::makePatch(const QString &url, const QUrlQuery &urlQuery)
{
    return this->makeRequest(url, urlQuery, NetworkRequestType::Patch);
}

void Helix::paginate(const QString &url, const QUrlQuery &baseQuery,
                     std::function<bool(const QJsonObject &)> onPage,
                     std::function<void(NetworkResult)> onError,
                     CancellationToken &&cancellationToken)
{
    auto onSuccess =
        std::make_shared<std::function<void(NetworkResult)>>(nullptr);
    // This is the actual callback passed to NetworkRequest.
    // It wraps the shared-ptr.
    auto onSuccessCb = [onSuccess](const auto &res) {
        return (*onSuccess)(res);
    };

    *onSuccess = [this, onPage = std::move(onPage), onError, onSuccessCb,
                  url{url}, baseQuery{baseQuery},
                  cancellationToken =
                      std::move(cancellationToken)](const NetworkResult &res) {
        if (cancellationToken.isCancelled())
        {
            return;
        }

        const auto json = res.parseJson();
        if (!onPage(json))
        {
            // The consumer doesn't want any more pages
            return;
        }

        auto cursor = json["pagination"_L1]["cursor"_L1].toString();
        if (cursor.isEmpty())
        {
            return;
        }

        auto query = baseQuery;
        query.removeAllQueryItems(u"after"_s);
        query.addQueryItem(u"after"_s, cursor);

        this->makeGet(url, query)
            .onSuccess(onSuccessCb)
            .onError(onError)
            .execute();
    };

    this->makeGet(url, baseQuery)
        .onSuccess(std::move(onSuccessCb))
        .onError(std::move(onError))
        .execute();
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
