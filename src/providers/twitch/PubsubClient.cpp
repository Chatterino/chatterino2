#include "providers/twitch/PubsubClient.hpp"

#include "providers/twitch/PubsubActions.hpp"
#include "providers/twitch/PubsubHelpers.hpp"
#include "util/Helpers.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <rapidjson/error/en.h>

#include <exception>
#include <thread>

#define TWITCH_PUBSUB_URL "wss://pubsub-edge.twitch.tv"

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

namespace chatterino {

static const char *pingPayload = "{\"type\":\"PING\"}";

static std::map<QString, std::string> sentMessages;

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

        for (const auto &topic : message["data"]["topics"].GetArray())
        {
            this->listeners_.emplace_back(
                Listener{topic.GetString(), false, false, false});
        }

        auto uuid = generateUuid();

        rj::set(message, "nonce", uuid);

        std::string payload = rj::stringify(message);
        sentMessages[uuid] = payload;

        this->send(payload.c_str());

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

        auto message = createUnlistenMessage(topics);

        auto uuid = generateUuid();

        rj::set(message, "nonce", generateUuid());

        std::string payload = rj::stringify(message);
        sentMessages[uuid] = payload;

        this->send(payload.c_str());
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
                         qDebug() << "No pong response, disconnect!";
                         // TODO(pajlada): Label this connection as "disconnect me"
                     }
                 });

        runAfter(this->websocketClient_.get_io_service(),
                 std::chrono::minutes(5), [self](auto timer) {
                     if (!self->started_)
                     {
                         return;
                     }

                     self->ping();  //
                 });
    }

    bool PubSubClient::send(const char *payload)
    {
        WebsocketErrorCode ec;
        this->websocketClient_.send(this->handle_, payload,
                                    websocketpp::frame::opcode::text, ec);

        if (ec)
        {
            qDebug() << "Error sending message" << payload << ":"
                     << ec.message().c_str();
            // TODO(pajlada): Check which error code happened and maybe
            // gracefully handle it

            return false;
        }

        return true;
    }

}  // namespace detail

PubSub::PubSub()
{
    qDebug() << "init PubSub";

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
            qDebug() << "Missing required args member";
            return;
        }

        const auto &args = data["args"];

        if (!args.IsArray())
        {
            qDebug() << "args member must be an array";
            return;
        }

        if (args.Size() == 0)
        {
            qDebug() << "Missing duration argument in slowmode on";
            return;
        }

        const auto &durationArg = args[0];

        if (!durationArg.IsString())
        {
            qDebug() << "Duration arg must be a string";
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

            if (!rj::getSafe(args[0], action.target.name))
            {
                return;
            }
        }
        catch (const std::runtime_error &ex)
        {
            qDebug() << "Error parsing moderation action:" << ex.what();
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
            qDebug() << "Error parsing moderation action mod: Unable to get "
                        "target_user_id";
            return;
        }

        // Load target name from message.data.target_user_login
        if (!getTargetUserName(data, action.target))
        {
            qDebug() << "Error parsing moderation action mod: Unable to get "
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

            if (!rj::getSafe(args[0], action.target.name))
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
            qDebug() << "Error parsing moderation action:" << ex.what();
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

            if (!rj::getSafe(args[0], action.target.name))
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
            qDebug() << "Error parsing moderation action:" << ex.what();
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

            if (!rj::getSafe(args[0], action.target.name))
            {
                return;
            }

            this->signals_.moderation.userUnbanned.invoke(action);
        }
        catch (const std::runtime_error &ex)
        {
            qDebug() << "Error parsing moderation action:" << ex.what();
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

            if (!rj::getSafe(args[0], action.target.name))
            {
                return;
            }

            this->signals_.moderation.userUnbanned.invoke(action);
        }
        catch (const std::runtime_error &ex)
        {
            qDebug() << "Error parsing moderation action:" << ex.what();
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

                if (!rj::getSafe(args[0], action.target.name))
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
                qDebug() << "Error parsing moderation action:" << ex.what();
            }
        };

    this->moderationActionHandlers["add_permitted_term"] =
        [this](const auto &data, const auto &roomID) {
            // This term got a pass through automod
            AutomodUserAction action(data, roomID);
            getCreatedByUser(data, action.source);

            try
            {
                const auto &args = getArgs(data);
                action.type = AutomodUserAction::AddPermitted;

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
                qDebug() << "Error parsing moderation action:" << ex.what();
            }
        };

    this->moderationActionHandlers["add_blocked_term"] =
        [this](const auto &data, const auto &roomID) {
            // A term has been added
            AutomodUserAction action(data, roomID);
            getCreatedByUser(data, action.source);

            try
            {
                const auto &args = getArgs(data);
                action.type = AutomodUserAction::AddBlocked;

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
                qDebug() << "Error parsing moderation action:" << ex.what();
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
                qDebug() << "Error parsing moderation action:" << ex.what();
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
                qDebug() << "Error parsing moderation action:" << ex.what();
            }
        };

    this->moderationActionHandlers["modified_automod_properties"] =
        [this](const auto &data, const auto &roomID) {
            // The automod settings got modified
            AutomodUserAction action(data, roomID);
            getCreatedByUser(data, action.source);
            action.type = AutomodUserAction::Properties;
            this->signals_.moderation.automodUserMessage.invoke(action);
        };

    this->moderationActionHandlers["denied_automod_message"] =
        [](const auto &data, const auto &roomID) {
            // This message got denied by a moderator
            // qDebug() << QString::fromStdString(rj::stringify(data));
        };

    this->moderationActionHandlers["approved_automod_message"] =
        [](const auto &data, const auto &roomID) {
            // This message got approved by a moderator
            // qDebug() << QString::fromStdString(rj::stringify(data));
        };

    this->websocketClient.set_access_channels(websocketpp::log::alevel::all);
    this->websocketClient.clear_access_channels(
        websocketpp::log::alevel::frame_payload);

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
    websocketpp::lib::error_code ec;
    auto con = this->websocketClient.get_connection(TWITCH_PUBSUB_URL, ec);

    if (ec)
    {
        qDebug() << "Unable to establish connection:" << ec.message().c_str();
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

    qDebug() << "Connection open!";
    websocketpp::lib::error_code ec;

    std::vector<QString> topics({topicFormat.arg(userID)});

    this->listen(createListenMessage(topics, account));

    if (ec)
    {
        qDebug() << "Unable to send message to websocket server:"
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

    auto topic = topicFormat.arg(userID).arg(channelID);

    if (this->isListeningToTopic(topic))
    {
        return;
    }

    qDebug() << "Listen to topic" << topic;

    this->listenToTopic(topic, account);
}

void PubSub::listenToChannelPointRewards(const QString &channelID,
                                         std::shared_ptr<TwitchAccount> account)
{
    static const QString topicFormat("community-points-channel-v1.%1");
    assert(!channelID.isEmpty());
    assert(account != nullptr);
    QString userID = account->getUserId();

    auto topic = topicFormat.arg(channelID);

    if (this->isListeningToTopic(topic))
    {
        return;
    }

    qDebug() << "Listen to topic" << topic;

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
    const std::string &payload = websocketMessage->get_payload();

    rapidjson::Document msg;

    rapidjson::ParseResult res = msg.Parse(payload.c_str());

    if (!res)
    {
        qDebug() << "Error parsing message '" << payload.c_str()
                 << "' from PubSub:" << rapidjson::GetParseError_En(res.Code());
        return;
    }

    if (!msg.IsObject())
    {
        qDebug() << "Error parsing message '" << payload.c_str()
                 << "' from PubSub. Root object is not an "
                    "object";
        return;
    }

    QString type;

    if (!rj::getSafe(msg, "type", type))
    {
        qDebug() << "Missing required string member `type` in message root";
        return;
    }

    if (type == "RESPONSE")
    {
        this->handleListenResponse(msg);
    }
    else if (type == "MESSAGE")
    {
        if (!msg.HasMember("data"))
        {
            qDebug() << "Missing required object member `data` in message root";
            return;
        }

        const auto &data = msg["data"];

        if (!data.IsObject())
        {
            qDebug() << "Member `data` must be an object";
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
        qDebug() << "Unknown message type:" << type;
    }
}

void PubSub::onConnectionOpen(WebsocketHandle hdl)
{
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
            it = this->requests.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void PubSub::onConnectionClose(WebsocketHandle hdl)
{
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
        qDebug() << "Exception caught in OnTLSInit:" << e.what();
    }

    return ctx;
}

void PubSub::handleListenResponse(const rapidjson::Document &msg)
{
    QString error;

    if (rj::getSafe(msg, "error", error))
    {
        QString nonce;
        rj::getSafe(msg, "nonce", nonce);

        if (error.isEmpty())
        {
            qDebug() << "Successfully listened to nonce" << nonce;
            // Nothing went wrong
            return;
        }

        qDebug() << "PubSub error:" << error << "on nonce" << nonce;
        return;
    }
}

void PubSub::handleMessageResponse(const rapidjson::Value &outerData)
{
    QString topic;

    if (!rj::getSafe(outerData, "topic", topic))
    {
        qDebug() << "Missing required string member `topic` in outerData";
        return;
    }

    std::string payload;

    if (!rj::getSafe(outerData, "message", payload))
    {
        qDebug() << "Expected string message in outerData";
        return;
    }

    rapidjson::Document msg;

    rapidjson::ParseResult res = msg.Parse(payload.c_str());

    if (!res)
    {
        qDebug() << "Error parsing message '" << payload.c_str()
                 << "' from PubSub:" << rapidjson::GetParseError_En(res.Code());
        return;
    }

    if (topic.startsWith("whispers."))
    {
        std::string whisperType;

        if (!rj::getSafe(msg, "type", whisperType))
        {
            qDebug() << "Bad whisper data";
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
            qDebug() << "Invalid whisper type:" << whisperType.c_str();
            return;
        }
    }
    else if (topic.startsWith("chat_moderator_actions."))
    {
        auto topicParts = topic.split(".");
        assert(topicParts.length() == 3);
        const auto &data = msg["data"];

        std::string moderationAction;

        if (!rj::getSafe(data, "moderation_action", moderationAction))
        {
            qDebug() << "Missing moderation action in data:"
                     << rj::stringify(data).c_str();
            return;
        }

        auto handlerIt = this->moderationActionHandlers.find(moderationAction);

        if (handlerIt == this->moderationActionHandlers.end())
        {
            qDebug() << "No handler found for moderation action"
                     << moderationAction.c_str();
            return;
        }

        // Invoke handler function
        handlerIt->second(data, topicParts[2]);
    }
    else if (topic.startsWith("community-points-channel-v1."))
    {
        std::string pointEventType;

        if (!rj::getSafe(msg, "type", pointEventType))
        {
            qDebug() << "Bad channel point event data";
            return;
        }

        if (pointEventType == "reward-redeemed")
        {
            if (!rj::getSafe(msg, "data", msg))
            {
                if (!rj::getSafe(msg, "redemption", msg))
                {
                    if (!rj::getSafe(msg, "reward", msg))
                    {
                        this->signals_.pointReward.redeemed.invoke(msg);
                    }
                    else
                    {
                        qDebug() << "No reward info found for redeemed reward";
                        return;
                    }
                }
                else
                {
                    qDebug() << "No redemption info found for redeemed reward";
                    return;
                }
            }
            else
            {
                qDebug() << "No data found for redeemed reward";
                return;
            }
        }
        else
        {
            qDebug() << "Invalid point event type:" << pointEventType.c_str();
        }
    }
    else
    {
        qDebug() << "Unknown topic:" << topic;
        return;
    }
}

void PubSub::runThread()
{
    qDebug() << "Start pubsub manager thread";
    this->websocketClient.run();
    qDebug() << "Done with pubsub manager thread";
}

}  // namespace chatterino
