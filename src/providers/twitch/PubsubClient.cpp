#include "providers/twitch/PubsubClient.hpp"

#include "providers/twitch/PubsubActions.hpp"
#include "providers/twitch/PubsubHelpers.hpp"
#include "singletons/Settings.hpp"
#include "util/DebugCount.hpp"
#include "util/Helpers.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <rapidjson/error/en.h>

#include <exception>
#include <iostream>
#include <thread>
#include "common/QLogging.hpp"

#define TWITCH_PUBSUB_URL "wss://pubsub-edge.twitch.tv"

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

namespace chatterino {

static const char *pingPayload = "{\"type\":\"PING\"}";

static std::map<QString, RequestMessage> sentListens;
static std::map<QString, RequestMessage> sentUnlistens;

namespace detail {

    PubSubClient::PubSubClient(WebsocketClient &websocketClient,
                               WebsocketHandle handle)
        : websocketClient_(websocketClient)
        , handle_(handle)
    {
    }

    void PubSubClient::start()
    {
        assert(!this->started_);

        this->started_ = true;

        this->ping();
    }

    void PubSubClient::stop()
    {
        assert(this->started_);

        this->started_ = false;
    }

    bool PubSubClient::listen(rapidjson::Document &message)
    {
        int numRequestedListens = message["data"]["topics"].Size();

        if (this->numListens_ + numRequestedListens > MAX_PUBSUB_LISTENS)
        {
            // This PubSubClient is already at its peak listens
            return false;
        }
        this->numListens_ += numRequestedListens;
        DebugCount::increase("PubSub topic pending listens",
                             numRequestedListens);

        for (const auto &topic : message["data"]["topics"].GetArray())
        {
            this->listeners_.emplace_back(
                Listener{topic.GetString(), false, false, false});
        }

        auto nonce = generateUuid();
        rj::set(message, "nonce", nonce);

        QString payload = rj::stringify(message);
        sentListens[nonce] = RequestMessage{payload, numRequestedListens};

        this->send(payload.toUtf8());

        return true;
    }

    void PubSubClient::unlistenPrefix(const QString &prefix)
    {
        std::vector<QString> topics;

        for (auto it = this->listeners_.begin(); it != this->listeners_.end();)
        {
            const auto &listener = *it;
            if (listener.topic.startsWith(prefix))
            {
                topics.push_back(listener.topic);
                it = this->listeners_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (topics.empty())
        {
            return;
        }

        int numRequestedUnlistens = topics.size();

        this->numListens_ -= numRequestedUnlistens;
        DebugCount::increase("PubSub topic pending unlistens",
                             numRequestedUnlistens);

        auto message = createUnlistenMessage(topics);

        auto nonce = generateUuid();
        rj::set(message, "nonce", nonce);

        QString payload = rj::stringify(message);
        sentUnlistens[nonce] = RequestMessage{payload, numRequestedUnlistens};

        this->send(payload.toUtf8());
    }

    void PubSubClient::handlePong()
    {
        assert(this->awaitingPong_);

        this->awaitingPong_ = false;
    }

    bool PubSubClient::isListeningToTopic(const QString &topic)
    {
        for (const auto &listener : this->listeners_)
        {
            if (listener.topic == topic)
            {
                return true;
            }
        }

        return false;
    }

    void PubSubClient::ping()
    {
        assert(this->started_);

        if (!this->send(pingPayload))
        {
            return;
        }

        this->awaitingPong_ = true;

        auto self = this->shared_from_this();

        runAfter(this->websocketClient_.get_io_service(),
                 std::chrono::seconds(15), [self](auto timer) {
                     if (!self->started_)
                     {
                         return;
                     }

                     if (self->awaitingPong_)
                     {
                         qCDebug(chatterinoPubsub)
                             << "No pong response, disconnect!";
                         // TODO(pajlada): Label this connection as "disconnect me"
                     }
                 });

        runAfter(this->websocketClient_.get_io_service(),
                 std::chrono::minutes(5), [self](auto timer) {
                     if (!self->started_)
                     {
                         return;
                     }

                     self->ping();
                 });
    }

    bool PubSubClient::send(const char *payload)
    {
        WebsocketErrorCode ec;
        this->websocketClient_.send(this->handle_, payload,
                                    websocketpp::frame::opcode::text, ec);

        if (ec)
        {
            qCDebug(chatterinoPubsub) << "Error sending message" << payload
                                      << ":" << ec.message().c_str();
            // TODO(pajlada): Check which error code happened and maybe
            // gracefully handle it

            return false;
        }

        return true;
    }

}  // namespace detail

PubSub::PubSub()
{
    qCDebug(chatterinoPubsub) << "init PubSub";

    this->moderationActionHandlers["clear"] = [this](const auto &data,
                                                     const auto &roomID) {
        ClearChatAction action(data, roomID);

        this->signals_.moderation.chatCleared.invoke(action);
    };

    this->moderationActionHandlers["slowoff"] = [this](const auto &data,
                                                       const auto &roomID) {
        ModeChangedAction action(data, roomID);

        action.mode = ModeChangedAction::Mode::Slow;
        action.state = ModeChangedAction::State::Off;

        this->signals_.moderation.modeChanged.invoke(action);
    };

    this->moderationActionHandlers["slow"] = [this](const auto &data,
                                                    const auto &roomID) {
        ModeChangedAction action(data, roomID);

        action.mode = ModeChangedAction::Mode::Slow;
        action.state = ModeChangedAction::State::On;

        if (!data.HasMember("args"))
        {
            qCDebug(chatterinoPubsub) << "Missing required args member";
            return;
        }

        const auto &args = data["args"];

        if (!args.IsArray())
        {
            qCDebug(chatterinoPubsub) << "args member must be an array";
            return;
        }

        if (args.Size() == 0)
        {
            qCDebug(chatterinoPubsub)
                << "Missing duration argument in slowmode on";
            return;
        }

        const auto &durationArg = args[0];

        if (!durationArg.IsString())
        {
            qCDebug(chatterinoPubsub) << "Duration arg must be a string";
            return;
        }

        bool ok;

        action.duration = QString(durationArg.GetString()).toUInt(&ok, 10);

        this->signals_.moderation.modeChanged.invoke(action);
    };

    this->moderationActionHandlers["r9kbetaoff"] = [this](const auto &data,
                                                          const auto &roomID) {
        ModeChangedAction action(data, roomID);

        action.mode = ModeChangedAction::Mode::R9K;
        action.state = ModeChangedAction::State::Off;

        this->signals_.moderation.modeChanged.invoke(action);
    };

    this->moderationActionHandlers["r9kbeta"] = [this](const auto &data,
                                                       const auto &roomID) {
        ModeChangedAction action(data, roomID);

        action.mode = ModeChangedAction::Mode::R9K;
        action.state = ModeChangedAction::State::On;

        this->signals_.moderation.modeChanged.invoke(action);
    };

    this->moderationActionHandlers["subscribersoff"] =
        [this](const auto &data, const auto &roomID) {
            ModeChangedAction action(data, roomID);

            action.mode = ModeChangedAction::Mode::SubscribersOnly;
            action.state = ModeChangedAction::State::Off;

            this->signals_.moderation.modeChanged.invoke(action);
        };

    this->moderationActionHandlers["subscribers"] = [this](const auto &data,
                                                           const auto &roomID) {
        ModeChangedAction action(data, roomID);

        action.mode = ModeChangedAction::Mode::SubscribersOnly;
        action.state = ModeChangedAction::State::On;

        this->signals_.moderation.modeChanged.invoke(action);
    };

    this->moderationActionHandlers["emoteonlyoff"] =
        [this](const auto &data, const auto &roomID) {
            ModeChangedAction action(data, roomID);

            action.mode = ModeChangedAction::Mode::EmoteOnly;
            action.state = ModeChangedAction::State::Off;

            this->signals_.moderation.modeChanged.invoke(action);
        };

    this->moderationActionHandlers["emoteonly"] = [this](const auto &data,
                                                         const auto &roomID) {
        ModeChangedAction action(data, roomID);

        action.mode = ModeChangedAction::Mode::EmoteOnly;
        action.state = ModeChangedAction::State::On;

        this->signals_.moderation.modeChanged.invoke(action);
    };

    this->moderationActionHandlers["unmod"] = [this](const auto &data,
                                                     const auto &roomID) {
        ModerationStateAction action(data, roomID);

        getTargetUser(data, action.target);

        try
        {
            const auto &args = getArgs(data);

            if (args.Size() < 1)
            {
                return;
            }

            if (!rj::getSafe(args[0], action.target.login))
            {
                return;
            }
        }
        catch (const std::runtime_error &ex)
        {
            qCDebug(chatterinoPubsub)
                << "Error parsing moderation action:" << ex.what();
        }

        action.modded = false;

        this->signals_.moderation.moderationStateChanged.invoke(action);
    };

    this->moderationActionHandlers["mod"] = [this](const auto &data,
                                                   const auto &roomID) {
        ModerationStateAction action(data, roomID);
        action.modded = true;

        QString innerType;
        if (rj::getSafe(data, "type", innerType) &&
            innerType == "chat_login_moderation")
        {
            // Don't display the old message type
            return;
        }

        if (!getTargetUser(data, action.target))
        {
            qCDebug(chatterinoPubsub)
                << "Error parsing moderation action mod: Unable to get "
                   "target_user_id";
            return;
        }

        // Load target name from message.data.target_user_login
        if (!getTargetUserName(data, action.target))
        {
            qCDebug(chatterinoPubsub)
                << "Error parsing moderation action mod: Unable to get "
                   "target_user_name";
            return;
        }

        this->signals_.moderation.moderationStateChanged.invoke(action);
    };

    this->moderationActionHandlers["timeout"] = [this](const auto &data,
                                                       const auto &roomID) {
        BanAction action(data, roomID);

        getCreatedByUser(data, action.source);
        getTargetUser(data, action.target);

        try
        {
            const auto &args = getArgs(data);

            if (args.Size() < 2)
            {
                return;
            }

            if (!rj::getSafe(args[0], action.target.login))
            {
                return;
            }

            QString durationString;
            if (!rj::getSafe(args[1], durationString))
            {
                return;
            }
            bool ok;
            action.duration = durationString.toUInt(&ok, 10);

            if (args.Size() >= 3)
            {
                if (!rj::getSafe(args[2], action.reason))
                {
                    return;
                }
            }

            this->signals_.moderation.userBanned.invoke(action);
        }
        catch (const std::runtime_error &ex)
        {
            qCDebug(chatterinoPubsub)
                << "Error parsing moderation action:" << ex.what();
        }
    };

    this->moderationActionHandlers["delete"] = [this](const auto &data,
                                                      const auto &roomID) {
        DeleteAction action(data, roomID);

        getCreatedByUser(data, action.source);
        getTargetUser(data, action.target);

        try
        {
            const auto &args = getArgs(data);

            if (args.Size() < 3)
            {
                return;
            }

            if (!rj::getSafe(args[0], action.target.login))
            {
                return;
            }

            if (!rj::getSafe(args[1], action.messageText))
            {
                return;
            }

            if (!rj::getSafe(args[2], action.messageId))
            {
                return;
            }

            this->signals_.moderation.messageDeleted.invoke(action);
        }
        catch (const std::runtime_error &ex)
        {
            qCDebug(chatterinoPubsub)
                << "Error parsing moderation action:" << ex.what();
        }
    };

    this->moderationActionHandlers["ban"] = [this](const auto &data,
                                                   const auto &roomID) {
        BanAction action(data, roomID);

        getCreatedByUser(data, action.source);
        getTargetUser(data, action.target);

        try
        {
            const auto &args = getArgs(data);

            if (args.Size() < 1)
            {
                return;
            }

            if (!rj::getSafe(args[0], action.target.login))
            {
                return;
            }

            if (args.Size() >= 2)
            {
                if (!rj::getSafe(args[1], action.reason))
                {
                    return;
                }
            }

            this->signals_.moderation.userBanned.invoke(action);
        }
        catch (const std::runtime_error &ex)
        {
            qCDebug(chatterinoPubsub)
                << "Error parsing moderation action:" << ex.what();
        }
    };

    this->moderationActionHandlers["unban"] = [this](const auto &data,
                                                     const auto &roomID) {
        UnbanAction action(data, roomID);

        getCreatedByUser(data, action.source);
        getTargetUser(data, action.target);

        action.previousState = UnbanAction::Banned;

        try
        {
            const auto &args = getArgs(data);

            if (args.Size() < 1)
            {
                return;
            }

            if (!rj::getSafe(args[0], action.target.login))
            {
                return;
            }

            this->signals_.moderation.userUnbanned.invoke(action);
        }
        catch (const std::runtime_error &ex)
        {
            qCDebug(chatterinoPubsub)
                << "Error parsing moderation action:" << ex.what();
        }
    };

    this->moderationActionHandlers["untimeout"] = [this](const auto &data,
                                                         const auto &roomID) {
        UnbanAction action(data, roomID);

        getCreatedByUser(data, action.source);
        getTargetUser(data, action.target);

        action.previousState = UnbanAction::TimedOut;

        try
        {
            const auto &args = getArgs(data);

            if (args.Size() < 1)
            {
                return;
            }

            if (!rj::getSafe(args[0], action.target.login))
            {
                return;
            }

            this->signals_.moderation.userUnbanned.invoke(action);
        }
        catch (const std::runtime_error &ex)
        {
            qCDebug(chatterinoPubsub)
                << "Error parsing moderation action:" << ex.what();
        }
    };

    this->moderationActionHandlers["automod_rejected"] =
        [this](const auto &data, const auto &roomID) {
            // Display the automod message and prompt the allow/deny
            AutomodAction action(data, roomID);

            getCreatedByUser(data, action.source);
            getTargetUser(data, action.target);

            try
            {
                const auto &args = getArgs(data);
                const auto &msgID = getMsgID(data);

                if (args.Size() < 1)
                {
                    return;
                }

                if (!rj::getSafe(args[0], action.target.login))
                {
                    return;
                }

                if (args.Size() >= 2)
                {
                    if (!rj::getSafe(args[1], action.message))
                    {
                        return;
                    }
                }

                if (args.Size() >= 3)
                {
                    if (!rj::getSafe(args[2], action.reason))
                    {
                        return;
                    }
                }

                if (!rj::getSafe(msgID, action.msgID))
                {
                    return;
                }

                this->signals_.moderation.automodMessage.invoke(action);
            }
            catch (const std::runtime_error &ex)
            {
                qCDebug(chatterinoPubsub)
                    << "Error parsing moderation action:" << ex.what();
            }
        };

    this->moderationActionHandlers["automod_message_rejected"] =
        [this](const auto &data, const auto &roomID) {
            AutomodInfoAction action(data, roomID);
            action.type = AutomodInfoAction::OnHold;
            this->signals_.moderation.automodInfoMessage.invoke(action);
        };

    this->moderationActionHandlers["automod_message_denied"] =
        [this](const auto &data, const auto &roomID) {
            AutomodInfoAction action(data, roomID);
            action.type = AutomodInfoAction::Denied;
            this->signals_.moderation.automodInfoMessage.invoke(action);
        };

    this->moderationActionHandlers["automod_message_approved"] =
        [this](const auto &data, const auto &roomID) {
            AutomodInfoAction action(data, roomID);
            action.type = AutomodInfoAction::Approved;
            this->signals_.moderation.automodInfoMessage.invoke(action);
        };

    this->channelTermsActionHandlers["add_permitted_term"] =
        [this](const auto &data, const auto &roomID) {
            // This term got a pass through automod
            AutomodUserAction action(data, roomID);
            getCreatedByUser(data, action.source);

            try
            {
                action.type = AutomodUserAction::AddPermitted;
                if (!rj::getSafe(data, "text", action.message))
                {
                    return;
                }

                if (!rj::getSafe(data, "requester_login", action.source.login))
                {
                    return;
                }

                this->signals_.moderation.automodUserMessage.invoke(action);
            }
            catch (const std::runtime_error &ex)
            {
                qCDebug(chatterinoPubsub)
                    << "Error parsing channel terms action:" << ex.what();
            }
        };

    this->channelTermsActionHandlers["add_blocked_term"] =
        [this](const auto &data, const auto &roomID) {
            // A term has been added
            AutomodUserAction action(data, roomID);
            getCreatedByUser(data, action.source);

            try
            {
                action.type = AutomodUserAction::AddBlocked;
                if (!rj::getSafe(data, "text", action.message))
                {
                    return;
                }

                if (!rj::getSafe(data, "requester_login", action.source.login))
                {
                    return;
                }

                this->signals_.moderation.automodUserMessage.invoke(action);
            }
            catch (const std::runtime_error &ex)
            {
                qCDebug(chatterinoPubsub)
                    << "Error parsing channel terms action:" << ex.what();
            }
        };

    this->moderationActionHandlers["delete_permitted_term"] =
        [this](const auto &data, const auto &roomID) {
            // This term got deleted
            AutomodUserAction action(data, roomID);
            getCreatedByUser(data, action.source);

            try
            {
                const auto &args = getArgs(data);
                action.type = AutomodUserAction::RemovePermitted;

                if (args.Size() < 1)
                {
                    return;
                }

                if (!rj::getSafe(args[0], action.message))
                {
                    return;
                }

                this->signals_.moderation.automodUserMessage.invoke(action);
            }
            catch (const std::runtime_error &ex)
            {
                qCDebug(chatterinoPubsub)
                    << "Error parsing moderation action:" << ex.what();
            }
        };
    this->channelTermsActionHandlers["delete_permitted_term"] =
        [this](const auto &data, const auto &roomID) {
            // This term got deleted
            AutomodUserAction action(data, roomID);
            getCreatedByUser(data, action.source);

            try
            {
                action.type = AutomodUserAction::RemovePermitted;
                if (!rj::getSafe(data, "text", action.message))
                {
                    return;
                }

                if (!rj::getSafe(data, "requester_login", action.source.login))
                {
                    return;
                }

                this->signals_.moderation.automodUserMessage.invoke(action);
            }
            catch (const std::runtime_error &ex)
            {
                qCDebug(chatterinoPubsub)
                    << "Error parsing channel terms action:" << ex.what();
            }
        };

    this->moderationActionHandlers["delete_blocked_term"] =
        [this](const auto &data, const auto &roomID) {
            // This term got deleted
            AutomodUserAction action(data, roomID);

            getCreatedByUser(data, action.source);

            try
            {
                const auto &args = getArgs(data);
                action.type = AutomodUserAction::RemoveBlocked;

                if (args.Size() < 1)
                {
                    return;
                }

                if (!rj::getSafe(args[0], action.message))
                {
                    return;
                }

                this->signals_.moderation.automodUserMessage.invoke(action);
            }
            catch (const std::runtime_error &ex)
            {
                qCDebug(chatterinoPubsub)
                    << "Error parsing moderation action:" << ex.what();
            }
        };
    this->channelTermsActionHandlers["delete_blocked_term"] =
        [this](const auto &data, const auto &roomID) {
            // This term got deleted
            AutomodUserAction action(data, roomID);

            getCreatedByUser(data, action.source);

            try
            {
                action.type = AutomodUserAction::RemoveBlocked;
                if (!rj::getSafe(data, "text", action.message))
                {
                    return;
                }

                if (!rj::getSafe(data, "requester_login", action.source.login))
                {
                    return;
                }

                this->signals_.moderation.automodUserMessage.invoke(action);
            }
            catch (const std::runtime_error &ex)
            {
                qCDebug(chatterinoPubsub)
                    << "Error parsing channel terms action:" << ex.what();
            }
        };

    // We don't get this one anymore or anything similiar
    // We need some new topic so we can listen
    //
    //this->moderationActionHandlers["modified_automod_properties"] =
    //    [this](const auto &data, const auto &roomID) {
    //        // The automod settings got modified
    //        AutomodUserAction action(data, roomID);
    //        getCreatedByUser(data, action.source);
    //        action.type = AutomodUserAction::Properties;
    //        this->signals_.moderation.automodUserMessage.invoke(action);
    //    };

    this->moderationActionHandlers["denied_automod_message"] =
        [](const auto &data, const auto &roomID) {
            // This message got denied by a moderator
            // qCDebug(chatterinoPubsub) << rj::stringify(data);
        };

    this->moderationActionHandlers["approved_automod_message"] =
        [](const auto &data, const auto &roomID) {
            // This message got approved by a moderator
            // qCDebug(chatterinoPubsub) << rj::stringify(data);
        };

    this->websocketClient.set_access_channels(websocketpp::log::alevel::all);
    this->websocketClient.clear_access_channels(
        websocketpp::log::alevel::frame_payload |
        websocketpp::log::alevel::frame_header);

    this->websocketClient.init_asio();

    // SSL Handshake
    this->websocketClient.set_tls_init_handler(
        bind(&PubSub::onTLSInit, this, ::_1));

    this->websocketClient.set_message_handler(
        bind(&PubSub::onMessage, this, ::_1, ::_2));
    this->websocketClient.set_open_handler(
        bind(&PubSub::onConnectionOpen, this, ::_1));
    this->websocketClient.set_close_handler(
        bind(&PubSub::onConnectionClose, this, ::_1));

    // Add an initial client
    this->addClient();
}

void PubSub::addClient()
{
    if (this->addingClient)
    {
        return;
    }

    this->addingClient = true;

    websocketpp::lib::error_code ec;
    auto con = this->websocketClient.get_connection(TWITCH_PUBSUB_URL, ec);

    if (ec)
    {
        qCDebug(chatterinoPubsub)
            << "Unable to establish connection:" << ec.message().c_str();
        return;
    }

    this->websocketClient.connect(con);
}

void PubSub::start()
{
    this->mainThread.reset(
        new std::thread(std::bind(&PubSub::runThread, this)));
}

void PubSub::listenToWhispers(std::shared_ptr<TwitchAccount> account)
{
    static const QString topicFormat("whispers.%1");

    assert(account != nullptr);

    auto userID = account->getUserId();

    qCDebug(chatterinoPubsub) << "Connection open!";
    websocketpp::lib::error_code ec;

    std::vector<QString> topics({topicFormat.arg(userID)});

    this->listen(createListenMessage(topics, account));

    if (ec)
    {
        qCDebug(chatterinoPubsub)
            << "Unable to send message to websocket server:"
            << ec.message().c_str();
        return;
    }
}

void PubSub::unlistenAllModerationActions()
{
    for (const auto &p : this->clients)
    {
        const auto &client = p.second;
        client->unlistenPrefix("chat_moderator_actions.");
    }
}

void PubSub::listenToChannelModerationActions(
    const QString &channelID, std::shared_ptr<TwitchAccount> account)
{
    static const QString topicFormat("chat_moderator_actions.%1.%2");
    assert(!channelID.isEmpty());
    assert(account != nullptr);
    QString userID = account->getUserId();
    if (userID.isEmpty())
        return;

    auto topic = topicFormat.arg(userID, channelID);

    if (this->isListeningToTopic(topic))
    {
        return;
    }

    qCDebug(chatterinoPubsub) << "Listen to topic" << topic;

    this->listenToTopic(topic, account);
}

void PubSub::listenToAutomod(const QString &channelID,
                             std::shared_ptr<TwitchAccount> account)
{
    static const QString topicFormat("automod-queue.%1.%2");
    assert(!channelID.isEmpty());
    assert(account != nullptr);
    QString userID = account->getUserId();
    if (userID.isEmpty())
        return;

    auto topic = topicFormat.arg(userID, channelID);

    if (this->isListeningToTopic(topic))
    {
        return;
    }

    qCDebug(chatterinoPubsub) << "Listen to topic" << topic;

    this->listenToTopic(topic, account);
}

void PubSub::listenToChannelPointRewards(const QString &channelID,
                                         std::shared_ptr<TwitchAccount> account)
{
    static const QString topicFormat("community-points-channel-v1.%1");
    assert(!channelID.isEmpty());
    assert(account != nullptr);

    auto topic = topicFormat.arg(channelID);

    if (this->isListeningToTopic(topic))
    {
        return;
    }
    qCDebug(chatterinoPubsub) << "Listen to topic" << topic;

    this->listenToTopic(topic, account);
}

void PubSub::listenToTopic(const QString &topic,
                           std::shared_ptr<TwitchAccount> account)
{
    auto message = createListenMessage({topic}, account);

    this->listen(std::move(message));
}

void PubSub::listen(rapidjson::Document &&msg)
{
    if (this->tryListen(msg))
    {
        return;
    }

    this->addClient();

    this->requests.emplace_back(
        std::make_unique<rapidjson::Document>(std::move(msg)));

    DebugCount::increase("PubSub topic backlog");
}

bool PubSub::tryListen(rapidjson::Document &msg)
{
    for (const auto &p : this->clients)
    {
        const auto &client = p.second;
        if (client->listen(msg))
        {
            return true;
        }
    }

    return false;
}

bool PubSub::isListeningToTopic(const QString &topic)
{
    for (const auto &p : this->clients)
    {
        const auto &client = p.second;
        if (client->isListeningToTopic(topic))
        {
            return true;
        }
    }

    return false;
}

void PubSub::onMessage(websocketpp::connection_hdl hdl,
                       WebsocketMessagePtr websocketMessage)
{
    const auto &payload =
        QString::fromStdString(websocketMessage->get_payload());

    rapidjson::Document msg;

    rapidjson::ParseResult res = msg.Parse(payload.toUtf8());

    if (!res)
    {
        qCDebug(chatterinoPubsub)
            << QString("Error parsing message '%1' from PubSub: %2")
                   .arg(payload, rapidjson::GetParseError_En(res.Code()));
        return;
    }

    if (!msg.IsObject())
    {
        qCDebug(chatterinoPubsub)
            << QString("Error parsing message '%1' from PubSub. Root object is "
                       "not an object")
                   .arg(payload);
        return;
    }

    QString type;

    if (!rj::getSafe(msg, "type", type))
    {
        qCDebug(chatterinoPubsub)
            << "Missing required string member `type` in message root";
        return;
    }

    if (type == "RESPONSE")
    {
        this->handleResponse(msg);
    }
    else if (type == "MESSAGE")
    {
        if (!msg.HasMember("data"))
        {
            qCDebug(chatterinoPubsub)
                << "Missing required object member `data` in message root";
            return;
        }

        const auto &data = msg["data"];

        if (!data.IsObject())
        {
            qCDebug(chatterinoPubsub) << "Member `data` must be an object";
            return;
        }

        this->handleMessageResponse(data);
    }
    else if (type == "PONG")
    {
        auto clientIt = this->clients.find(hdl);

        // If this assert goes off, there's something wrong with the connection
        // creation/preserving code KKona
        assert(clientIt != this->clients.end());

        auto &client = *clientIt;

        client.second->handlePong();
    }
    else
    {
        qCDebug(chatterinoPubsub) << "Unknown message type:" << type;
    }
}

void PubSub::onConnectionOpen(WebsocketHandle hdl)
{
    DebugCount::increase("PubSub connections");
    this->addingClient = false;

    auto client =
        std::make_shared<detail::PubSubClient>(this->websocketClient, hdl);

    // We separate the starting from the constructor because we will want to use
    // shared_from_this
    client->start();

    this->clients.emplace(hdl, client);

    this->connected.invoke();

    for (auto it = this->requests.begin(); it != this->requests.end();)
    {
        const auto &request = *it;
        if (client->listen(*request))
        {
            DebugCount::decrease("PubSub topic backlog");
            it = this->requests.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (!this->requests.empty())
    {
        this->addClient();
    }
}

void PubSub::onConnectionClose(WebsocketHandle hdl)
{
    DebugCount::decrease("PubSub connections");
    auto clientIt = this->clients.find(hdl);

    // If this assert goes off, there's something wrong with the connection
    // creation/preserving code KKona
    assert(clientIt != this->clients.end());

    auto &client = clientIt->second;

    client->stop();

    this->clients.erase(clientIt);

    this->connected.invoke();
}

PubSub::WebsocketContextPtr PubSub::onTLSInit(websocketpp::connection_hdl hdl)
{
    WebsocketContextPtr ctx(
        new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1));

    try
    {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::single_dh_use);
    }
    catch (const std::exception &e)
    {
        qCDebug(chatterinoPubsub)
            << "Exception caught in OnTLSInit:" << e.what();
    }

    return ctx;
}

void PubSub::handleResponse(const rapidjson::Document &msg)
{
    QString error;

    if (!rj::getSafe(msg, "error", error))
        return;

    QString nonce;
    rj::getSafe(msg, "nonce", nonce);

    const bool failed = !error.isEmpty();

    if (failed)
    {
        qCDebug(chatterinoPubsub)
            << QString("Error %1 on nonce %2").arg(error, nonce);
    }

    if (auto it = sentListens.find(nonce); it != sentListens.end())
    {
        this->handleListenResponse(it->second, failed);
        return;
    }

    if (auto it = sentUnlistens.find(nonce); it != sentUnlistens.end())
    {
        this->handleUnlistenResponse(it->second, failed);
        return;
    }

    qCDebug(chatterinoPubsub)
        << "Response on unused" << nonce << "client/topic listener mismatch?";
}

void PubSub::handleListenResponse(const RequestMessage &msg, bool failed)
{
    DebugCount::decrease("PubSub topic pending listens", msg.topicCount);
    if (failed)
    {
        DebugCount::increase("PubSub topic failed listens", msg.topicCount);
    }
    else
    {
        DebugCount::increase("PubSub topic listening", msg.topicCount);
    }
}

void PubSub::handleUnlistenResponse(const RequestMessage &msg, bool failed)
{
    DebugCount::decrease("PubSub topic pending unlistens", msg.topicCount);
    if (failed)
    {
        DebugCount::increase("PubSub topic failed unlistens", msg.topicCount);
    }
    else
    {
        DebugCount::decrease("PubSub topic listening", msg.topicCount);
    }
}

void PubSub::handleMessageResponse(const rapidjson::Value &outerData)
{
    QString topic;
    qCDebug(chatterinoPubsub) << rj::stringify(outerData);

    if (!rj::getSafe(outerData, "topic", topic))
    {
        qCDebug(chatterinoPubsub)
            << "Missing required string member `topic` in outerData";
        return;
    }

    QString payload;

    if (!rj::getSafe(outerData, "message", payload))
    {
        qCDebug(chatterinoPubsub) << "Expected string message in outerData";
        return;
    }

    rapidjson::Document msg;

    rapidjson::ParseResult res = msg.Parse(payload.toUtf8());

    if (!res)
    {
        qCDebug(chatterinoPubsub)
            << QString("Error parsing message '%1' from PubSub: %2")
                   .arg(payload, rapidjson::GetParseError_En(res.Code()));
        return;
    }

    if (topic.startsWith("whispers."))
    {
        QString whisperType;

        if (!rj::getSafe(msg, "type", whisperType))
        {
            qCDebug(chatterinoPubsub) << "Bad whisper data";
            return;
        }

        if (whisperType == "whisper_received")
        {
            this->signals_.whisper.received.invoke(msg);
        }
        else if (whisperType == "whisper_sent")
        {
            this->signals_.whisper.sent.invoke(msg);
        }
        else if (whisperType == "thread")
        {
            // Handle thread?
        }
        else
        {
            qCDebug(chatterinoPubsub) << "Invalid whisper type:" << whisperType;
            return;
        }
    }
    else if (topic.startsWith("chat_moderator_actions."))
    {
        auto topicParts = topic.split(".");
        assert(topicParts.length() == 3);
        const auto &data = msg["data"];

        QString moderationEventType;

        if (!rj::getSafe(msg, "type", moderationEventType))
        {
            qCDebug(chatterinoPubsub) << "Bad moderator event data";
            return;
        }
        if (moderationEventType == "moderation_action")
        {
            QString moderationAction;

            if (!rj::getSafe(data, "moderation_action", moderationAction))
            {
                qCDebug(chatterinoPubsub)
                    << "Missing moderation action in data:"
                    << rj::stringify(data);
                return;
            }

            auto handlerIt =
                this->moderationActionHandlers.find(moderationAction);

            if (handlerIt == this->moderationActionHandlers.end())
            {
                qCDebug(chatterinoPubsub)
                    << "No handler found for moderation action"
                    << moderationAction;
                return;
            }
            // Invoke handler function
            handlerIt->second(data, topicParts[2]);
        }
        else if (moderationEventType == "channel_terms_action")
        {
            QString channelTermsAction;

            if (!rj::getSafe(data, "type", channelTermsAction))
            {
                qCDebug(chatterinoPubsub)
                    << "Missing channel terms action in data:"
                    << rj::stringify(data);
                return;
            }

            auto handlerIt =
                this->channelTermsActionHandlers.find(channelTermsAction);

            if (handlerIt == this->channelTermsActionHandlers.end())
            {
                qCDebug(chatterinoPubsub)
                    << "No handler found for channel terms action"
                    << channelTermsAction;
                return;
            }
            // Invoke handler function
            handlerIt->second(data, topicParts[2]);
        }
    }
    else if (topic.startsWith("community-points-channel-v1."))
    {
        QString pointEventType;
        if (!rj::getSafe(msg, "type", pointEventType))
        {
            qCDebug(chatterinoPubsub) << "Bad channel point event data";
            return;
        }

        if (pointEventType == "reward-redeemed")
        {
            if (!rj::getSafeObject(msg, "data", msg))
            {
                qCDebug(chatterinoPubsub)
                    << "No data found for redeemed reward";
                return;
            }
            if (!rj::getSafeObject(msg, "redemption", msg))
            {
                qCDebug(chatterinoPubsub)
                    << "No redemption info found for redeemed reward";
                return;
            }
            this->signals_.pointReward.redeemed.invoke(msg);
        }
        else
        {
            qCDebug(chatterinoPubsub)
                << "Invalid point event type:" << pointEventType;
        }
    }
    else if (topic.startsWith("automod-queue."))
    {
        auto topicParts = topic.split(".");
        assert(topicParts.length() == 3);
        auto &data = msg["data"];

        QString automodEventType;
        if (!rj::getSafe(msg, "type", automodEventType))
        {
            qCDebug(chatterinoPubsub) << "Bad automod event data";
            return;
        }

        if (automodEventType == "automod_caught_message")
        {
            QString status;
            if (!rj::getSafe(data, "status", status))
            {
                qCDebug(chatterinoPubsub) << "Failed to get status";
                return;
            }
            if (status == "PENDING")
            {
                AutomodAction action(data, topicParts[2]);
                rapidjson::Value classification;
                if (!rj::getSafeObject(data, "content_classification",
                                       classification))
                {
                    qCDebug(chatterinoPubsub)
                        << "Failed to get content_classification";
                    return;
                }

                QString contentCategory;
                if (!rj::getSafe(classification, "category", contentCategory))
                {
                    qCDebug(chatterinoPubsub)
                        << "Failed to get content category";
                    return;
                }
                int contentLevel;
                if (!rj::getSafe(classification, "level", contentLevel))
                {
                    qCDebug(chatterinoPubsub) << "Failed to get content level";
                    return;
                }
                action.reason = QString("%1 level %2")
                                    .arg(contentCategory)
                                    .arg(contentLevel);

                rapidjson::Value messageData;
                if (!rj::getSafeObject(data, "message", messageData))
                {
                    qCDebug(chatterinoPubsub) << "Failed to get message data";
                    return;
                }

                rapidjson::Value messageContent;
                if (!rj::getSafeObject(messageData, "content", messageContent))
                {
                    qCDebug(chatterinoPubsub)
                        << "Failed to get message content";
                    return;
                }
                if (!rj::getSafe(messageData, "id", action.msgID))
                {
                    qCDebug(chatterinoPubsub) << "Failed to get message id";
                    return;
                }

                if (!rj::getSafe(messageContent, "text", action.message))
                {
                    qCDebug(chatterinoPubsub) << "Failed to get message text";
                    return;
                }

                // this message also contains per-word automod data, which could be implemented

                // extract sender data manually because Twitch loves not being consistent
                rapidjson::Value senderData;
                if (!rj::getSafeObject(messageData, "sender", senderData))
                {
                    qCDebug(chatterinoPubsub) << "Failed to get sender";
                    return;
                }
                QString senderId;
                if (!rj::getSafe(senderData, "user_id", senderId))
                {
                    qCDebug(chatterinoPubsub) << "Failed to get sender user id";
                    return;
                }
                QString senderLogin;
                if (!rj::getSafe(senderData, "login", senderLogin))
                {
                    qCDebug(chatterinoPubsub) << "Failed to get sender login";
                    return;
                }
                QString senderDisplayName = senderLogin;
                bool hasLocalizedName = false;
                if (rj::getSafe(senderData, "display_name", senderDisplayName))
                {
                    // check for non-ascii display names
                    if (QString::compare(senderLogin, senderDisplayName,
                                         Qt::CaseInsensitive) != 0)
                    {
                        hasLocalizedName = true;
                    }
                }
                QColor senderColor;
                QString senderColor_;
                if (rj::getSafe(senderData, "chat_color", senderColor_))
                {
                    senderColor = QColor(senderColor_);
                }
                else if (getSettings()->colorizeNicknames)
                {
                    // color may be not present if user is a grey-name
                    senderColor = getRandomColor(senderId);
                }
                // handle username style based on prefered setting
                switch (getSettings()->usernameDisplayMode.getValue())
                {
                    case UsernameDisplayMode::Username: {
                        if (hasLocalizedName)
                        {
                            senderDisplayName = senderLogin;
                        }
                        break;
                    }
                    case UsernameDisplayMode::LocalizedName: {
                        break;
                    }
                    case UsernameDisplayMode::UsernameAndLocalizedName: {
                        if (hasLocalizedName)
                        {
                            senderDisplayName = QString("%1(%2)").arg(
                                senderLogin, senderDisplayName);
                        }
                        break;
                    }
                }

                action.target = ActionUser{senderId, senderLogin,
                                           senderDisplayName, senderColor};
                this->signals_.moderation.automodMessage.invoke(action);
            }
            // "ALLOWED" and "DENIED" statuses remain unimplemented
            // They are versions of automod_message_(denied|approved) but for mods.
        }
    }
    else
    {
        qCDebug(chatterinoPubsub) << "Unknown topic:" << topic;
        return;
    }
}

void PubSub::runThread()
{
    qCDebug(chatterinoPubsub) << "Start pubsub manager thread";
    this->websocketClient.run();
    qCDebug(chatterinoPubsub) << "Done with pubsub manager thread";
}

}  // namespace chatterino
