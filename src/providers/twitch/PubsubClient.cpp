#include "providers/twitch/PubsubClient.hpp"

#include "debug/Log.hpp"
#include "providers/twitch/PubsubActions.hpp"
#include "providers/twitch/PubsubHelpers.hpp"
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

        if (this->numListens_ + numRequestedListens > MAX_PUBSUB_LISTENS) {
            // This PubSubClient is already at its peak listens
            return false;
        }

        this->numListens_ += numRequestedListens;

        for (const auto &topic : message["data"]["topics"].GetArray()) {
            this->listeners_.emplace_back(
                Listener{topic.GetString(), false, false, false});
        }

        auto uuid = CreateUUID();

        rj::set(message, "nonce", uuid);

        std::string payload = rj::stringify(message);
        sentMessages[uuid] = payload;

        this->send(payload.c_str());

        return true;
    }

    void PubSubClient::unlistenPrefix(const std::string &prefix)
    {
        std::vector<std::string> topics;

        for (auto it = this->listeners_.begin();
             it != this->listeners_.end();) {
            const auto &listener = *it;
            if (listener.topic.find(prefix) == 0) {
                topics.push_back(listener.topic);
                it = this->listeners_.erase(it);
            } else {
                ++it;
            }
        }

        if (topics.empty()) {
            return;
        }

        auto message = createUnlistenMessage(topics);

        auto uuid = CreateUUID();

        rj::set(message, "nonce", CreateUUID());

        std::string payload = rj::stringify(message);
        sentMessages[uuid] = payload;

        this->send(payload.c_str());
    }

    void PubSubClient::handlePong()
    {
        assert(this->awaitingPong_);

        log("Got pong!");

        this->awaitingPong_ = false;
    }

    bool PubSubClient::isListeningToTopic(const std::string &payload)
    {
        for (const auto &listener : this->listeners_) {
            if (listener.topic == payload) {
                return true;
            }
        }

        return false;
    }

    void PubSubClient::ping()
    {
        assert(this->started_);

        if (!this->send(pingPayload)) {
            return;
        }

        this->awaitingPong_ = true;

        auto self = this->shared_from_this();

        runAfter(this->websocketClient_.get_io_service(),
                 std::chrono::seconds(15), [self](auto timer) {
                     if (!self->started_) {
                         return;
                     }

                     if (self->awaitingPong_) {
                         log("No pong respnose, disconnect!");
                         // TODO(pajlada): Label this connection as "disconnect
                         // me"
                     }
                 });

        runAfter(this->websocketClient_.get_io_service(),
                 std::chrono::minutes(5), [self](auto timer) {
                     if (!self->started_) {
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

        if (ec) {
            log("Error sending message {}: {}", payload, ec.message());
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

        if (!data.HasMember("args")) {
            log("Missing required args member");
            return;
        }

        const auto &args = data["args"];

        if (!args.IsArray()) {
            log("args member must be an array");
            return;
        }

        if (args.Size() == 0) {
            log("Missing duration argument in slowmode on");
            return;
        }

        const auto &durationArg = args[0];

        if (!durationArg.IsString()) {
            log("Duration arg must be a string");
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

        try {
            const auto &args = getArgs(data);

            if (args.Size() < 1) {
                return;
            }

            if (!rj::getSafe(args[0], action.target.name)) {
                return;
            }
        } catch (const std::runtime_error &ex) {
            log("Error parsing moderation action: {}", ex.what());
        }

        action.modded = false;

        this->signals_.moderation.moderationStateChanged.invoke(action);
    };

    this->moderationActionHandlers["mod"] = [this](const auto &data,
                                                   const auto &roomID) {
        ModerationStateAction action(data, roomID);

        getTargetUser(data, action.target);

        try {
            const auto &args = getArgs(data);

            if (args.Size() < 1) {
                return;
            }

            if (!rj::getSafe(args[0], action.target.name)) {
                return;
            }
        } catch (const std::runtime_error &ex) {
            log("Error parsing moderation action: {}", ex.what());
        }

        action.modded = true;

        this->signals_.moderation.moderationStateChanged.invoke(action);
    };

    this->moderationActionHandlers["timeout"] = [this](const auto &data,
                                                       const auto &roomID) {
        BanAction action(data, roomID);

        getCreatedByUser(data, action.source);
        getTargetUser(data, action.target);

        try {
            const auto &args = getArgs(data);

            if (args.Size() < 2) {
                return;
            }

            if (!rj::getSafe(args[0], action.target.name)) {
                return;
            }

            QString durationString;
            if (!rj::getSafe(args[1], durationString)) {
                return;
            }
            bool ok;
            action.duration = durationString.toUInt(&ok, 10);

            if (args.Size() >= 3) {
                if (!rj::getSafe(args[2], action.reason)) {
                    return;
                }
            }

            this->signals_.moderation.userBanned.invoke(action);
        } catch (const std::runtime_error &ex) {
            log("Error parsing moderation action: {}", ex.what());
        }
    };

    this->moderationActionHandlers["ban"] = [this](const auto &data,
                                                   const auto &roomID) {
        BanAction action(data, roomID);

        getCreatedByUser(data, action.source);
        getTargetUser(data, action.target);

        try {
            const auto &args = getArgs(data);

            if (args.Size() < 1) {
                return;
            }

            if (!rj::getSafe(args[0], action.target.name)) {
                return;
            }

            if (args.Size() >= 2) {
                if (!rj::getSafe(args[1], action.reason)) {
                    return;
                }
            }

            this->signals_.moderation.userBanned.invoke(action);
        } catch (const std::runtime_error &ex) {
            log("Error parsing moderation action: {}", ex.what());
        }
    };

    this->moderationActionHandlers["unban"] = [this](const auto &data,
                                                     const auto &roomID) {
        UnbanAction action(data, roomID);

        getCreatedByUser(data, action.source);
        getTargetUser(data, action.target);

        action.previousState = UnbanAction::Banned;

        try {
            const auto &args = getArgs(data);

            if (args.Size() < 1) {
                return;
            }

            if (!rj::getSafe(args[0], action.target.name)) {
                return;
            }

            this->signals_.moderation.userUnbanned.invoke(action);
        } catch (const std::runtime_error &ex) {
            log("Error parsing moderation action: {}", ex.what());
        }
    };

    this->moderationActionHandlers["untimeout"] = [this](const auto &data,
                                                         const auto &roomID) {
        UnbanAction action(data, roomID);

        getCreatedByUser(data, action.source);
        getTargetUser(data, action.target);

        action.previousState = UnbanAction::TimedOut;

        try {
            const auto &args = getArgs(data);

            if (args.Size() < 1) {
                return;
            }

            if (!rj::getSafe(args[0], action.target.name)) {
                return;
            }

            this->signals_.moderation.userUnbanned.invoke(action);
        } catch (const std::runtime_error &ex) {
            log("Error parsing moderation action: {}", ex.what());
        }
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

    if (ec) {
        log("Unable to establish connection: {}", ec.message());
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
    assert(account != nullptr);

    std::string userID = account->getUserId().toStdString();

    log("Connection open!");
    websocketpp::lib::error_code ec;

    std::vector<std::string> topics({"whispers." + userID});

    this->listen(createListenMessage(topics, account));

    if (ec) {
        log("Unable to send message to websocket server: {}", ec.message());
        return;
    }
}

void PubSub::unlistenAllModerationActions()
{
    for (const auto &p : this->clients) {
        const auto &client = p.second;
        client->unlistenPrefix("chat_moderator_actions.");
    }
}

void PubSub::listenToChannelModerationActions(
    const QString &channelID, std::shared_ptr<TwitchAccount> account)
{
    assert(!channelID.isEmpty());
    assert(account != nullptr);
    QString userID = account->getUserId();
    if (userID.isEmpty()) return;

    std::string topic(fS("chat_moderator_actions.{}.{}", userID, channelID));

    if (this->isListeningToTopic(topic)) {
        log("We are already listening to topic {}", topic);
        return;
    }

    log("Listen to topic {}", topic);

    this->listenToTopic(topic, account);
}

void PubSub::listenToTopic(const std::string &topic,
                           std::shared_ptr<TwitchAccount> account)
{
    auto message = createListenMessage({topic}, account);

    this->listen(std::move(message));
}

void PubSub::listen(rapidjson::Document &&msg)
{
    if (this->tryListen(msg)) {
        log("Successfully listened!");
        return;
    }

    log("Added to the back of the queue");
    this->requests.emplace_back(
        std::make_unique<rapidjson::Document>(std::move(msg)));
}

bool PubSub::tryListen(rapidjson::Document &msg)
{
    log("tryListen with {} clients", this->clients.size());
    for (const auto &p : this->clients) {
        const auto &client = p.second;
        if (client->listen(msg)) {
            return true;
        }
    }

    return false;
}

bool PubSub::isListeningToTopic(const std::string &topic)
{
    for (const auto &p : this->clients) {
        const auto &client = p.second;
        if (client->isListeningToTopic(topic)) {
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

    if (!res) {
        log("Error parsing message '{}' from PubSub: {}", payload,
            rapidjson::GetParseError_En(res.Code()));
        return;
    }

    if (!msg.IsObject()) {
        log("Error parsing message '{}' from PubSub. Root object is not an "
            "object",
            payload);
        return;
    }

    std::string type;

    if (!rj::getSafe(msg, "type", type)) {
        log("Missing required string member `type` in message root");
        return;
    }

    if (type == "RESPONSE") {
        this->handleListenResponse(msg);
    } else if (type == "MESSAGE") {
        if (!msg.HasMember("data")) {
            log("Missing required object member `data` in message root");
            return;
        }

        const auto &data = msg["data"];

        if (!data.IsObject()) {
            log("Member `data` must be an object");
            return;
        }

        this->handleMessageResponse(data);
    } else if (type == "PONG") {
        auto clientIt = this->clients.find(hdl);

        // If this assert goes off, there's something wrong with the connection
        // creation/preserving code KKona
        assert(clientIt != this->clients.end());

        auto &client = *clientIt;

        client.second->handlePong();
    } else {
        log("Unknown message type: {}", type);
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

    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::single_dh_use);
    } catch (const std::exception &e) {
        log("Exception caught in OnTLSInit: {}", e.what());
    }

    return ctx;
}

void PubSub::handleListenResponse(const rapidjson::Document &msg)
{
    std::string error;

    if (rj::getSafe(msg, "error", error)) {
        std::string nonce;
        rj::getSafe(msg, "nonce", nonce);

        if (error.empty()) {
            log("Successfully listened to nonce {}", nonce);
            // Nothing went wrong
            return;
        }

        log("PubSub error: {} on nonce {}", error, nonce);
        return;
    }
}

void PubSub::handleMessageResponse(const rapidjson::Value &outerData)
{
    QString topic;

    if (!rj::getSafe(outerData, "topic", topic)) {
        log("Missing required string member `topic` in outerData");
        return;
    }

    std::string payload;

    if (!rj::getSafe(outerData, "message", payload)) {
        log("Expected string message in outerData");
        return;
    }

    rapidjson::Document msg;

    rapidjson::ParseResult res = msg.Parse(payload.c_str());

    if (!res) {
        log("Error parsing message '{}' from PubSub: {}", payload,
            rapidjson::GetParseError_En(res.Code()));
        return;
    }

    if (topic.startsWith("whispers.")) {
        std::string whisperType;

        if (!rj::getSafe(msg, "type", whisperType)) {
            log("Bad whisper data");
            return;
        }

        if (whisperType == "whisper_received") {
            this->signals_.whisper.received.invoke(msg);
        } else if (whisperType == "whisper_sent") {
            this->signals_.whisper.sent.invoke(msg);
        } else if (whisperType == "thread") {
            // Handle thread?
        } else {
            log("Invalid whisper type: {}", whisperType);
            assert(false);
            return;
        }
    } else if (topic.startsWith("chat_moderator_actions.")) {
        auto topicParts = topic.split(".");
        assert(topicParts.length() == 3);
        const auto &data = msg["data"];

        std::string moderationAction;

        if (!rj::getSafe(data, "moderation_action", moderationAction)) {
            log("Missing moderation action in data: {}", rj::stringify(data));
            return;
        }

        auto handlerIt = this->moderationActionHandlers.find(moderationAction);

        if (handlerIt == this->moderationActionHandlers.end()) {
            log("No handler found for moderation action {}", moderationAction);
            return;
        }

        // Invoke handler function
        handlerIt->second(data, topicParts[2]);
    } else {
        log("Unknown topic: {}", topic);
        return;
    }
}

void PubSub::runThread()
{
    log("Start pubsub manager thread");
    this->websocketClient.run();
    log("Done with pubsub manager thread");
}

}  // namespace chatterino
