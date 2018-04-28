#include "providers/twitch/pubsub.hpp"

#include "debug/log.hpp"
#include "providers/twitch/pubsubactions.hpp"
#include "providers/twitch/pubsubhelpers.hpp"
#include "singletons/accountmanager.hpp"
#include "util/rapidjson-helpers.hpp"

#include <rapidjson/error/en.h>

#include <exception>
#include <thread>

#define TWITCH_PUBSUB_URL "wss://pubsub-edge.twitch.tv"

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

namespace chatterino {
namespace providers {
namespace twitch {

static const char *pingPayload = "{\"type\":\"PING\"}";

static std::map<std::string, std::string> sentMessages;

namespace detail {

PubSubClient::PubSubClient(WebsocketClient &_websocketClient, WebsocketHandle _handle)
    : websocketClient(_websocketClient)
    , handle(_handle)
{
}

void PubSubClient::Start()
{
    assert(!this->started);

    this->started = true;

    this->Ping();
}

void PubSubClient::Stop()
{
    assert(this->started);

    this->started = false;
}

bool PubSubClient::Listen(rapidjson::Document &message)
{
    int numRequestedListens = message["data"]["topics"].Size();

    if (this->numListens + numRequestedListens > MAX_PUBSUB_LISTENS) {
        // This PubSubClient is already at its peak listens
        return false;
    }

    this->numListens += numRequestedListens;

    for (const auto &topic : message["data"]["topics"].GetArray()) {
        this->listeners.emplace_back(Listener{topic.GetString(), false, false, false});
    }

    auto uuid = CreateUUID();

    rj::set(message, "nonce", uuid);

    std::string payload = Stringify(message);
    sentMessages[uuid.toStdString()] = payload;

    this->Send(payload.c_str());

    return true;
}

void PubSubClient::UnlistenPrefix(const std::string &prefix)
{
    std::vector<std::string> topics;

    for (auto it = this->listeners.begin(); it != this->listeners.end();) {
        const auto &listener = *it;
        if (listener.topic.find(prefix) == 0) {
            topics.push_back(listener.topic);
            it = this->listeners.erase(it);
        } else {
            ++it;
        }
    }

    if (topics.empty()) {
        return;
    }

    auto message = CreateUnlistenMessage(topics);

    auto uuid = CreateUUID();

    rj::set(message, "nonce", CreateUUID());

    std::string payload = Stringify(message);
    sentMessages[uuid.toStdString()] = payload;

    this->Send(payload.c_str());
}

void PubSubClient::HandlePong()
{
    assert(this->awaitingPong);

    debug::Log("Got pong!");

    this->awaitingPong = false;
}

bool PubSubClient::isListeningToTopic(const std::string &payload)
{
    for (const auto &listener : this->listeners) {
        if (listener.topic == payload) {
            return true;
        }
    }

    return false;
}

void PubSubClient::Ping()
{
    assert(this->started);

    if (!this->Send(pingPayload)) {
        return;
    }

    this->awaitingPong = true;

    auto self = this->shared_from_this();

    RunAfter(this->websocketClient.get_io_service(), std::chrono::seconds(15), [self](auto timer) {
        if (!self->started) {
            return;
        }

        if (self->awaitingPong) {
            debug::Log("No pong respnose, disconnect!");
            // TODO(pajlada): Label this connection as "disconnect me"
        }
    });

    RunAfter(this->websocketClient.get_io_service(), std::chrono::minutes(5), [self](auto timer) {
        if (!self->started) {
            return;
        }

        self->Ping();  //
    });
}

bool PubSubClient::Send(const char *payload)
{
    WebsocketErrorCode ec;
    this->websocketClient.send(this->handle, payload, websocketpp::frame::opcode::text, ec);

    if (ec) {
        debug::Log("Error sending message {}: {}", payload, ec.message());
        // TODO(pajlada): Check which error code happened and maybe gracefully handle it

        return false;
    }

    return true;
}

}  // namespace detail

PubSub::PubSub()
{
    qDebug() << "init PubSub";

    this->moderationActionHandlers["clear"] = [this](const auto &data, const auto &roomID) {
        ClearChatAction action(data, roomID);

        this->sig.moderation.chatCleared.invoke(action);
    };

    this->moderationActionHandlers["slowoff"] = [this](const auto &data, const auto &roomID) {
        ModeChangedAction action(data, roomID);

        action.mode = ModeChangedAction::Mode::Slow;
        action.state = ModeChangedAction::State::Off;

        this->sig.moderation.modeChanged.invoke(action);
    };

    this->moderationActionHandlers["slow"] = [this](const auto &data, const auto &roomID) {
        ModeChangedAction action(data, roomID);

        action.mode = ModeChangedAction::Mode::Slow;
        action.state = ModeChangedAction::State::On;

        if (!data.HasMember("args")) {
            debug::Log("Missing required args member");
            return;
        }

        const auto &args = data["args"];

        if (!args.IsArray()) {
            debug::Log("args member must be an array");
            return;
        }

        if (args.Size() == 0) {
            debug::Log("Missing duration argument in slowmode on");
            return;
        }

        const auto &durationArg = args[0];

        if (!durationArg.IsString()) {
            debug::Log("Duration arg must be a string");
            return;
        }

        bool ok;

        action.args.duration = QString(durationArg.GetString()).toUInt(&ok, 10);

        this->sig.moderation.modeChanged.invoke(action);
    };

    this->moderationActionHandlers["r9kbetaoff"] = [this](const auto &data, const auto &roomID) {
        ModeChangedAction action(data, roomID);

        action.mode = ModeChangedAction::Mode::R9K;
        action.state = ModeChangedAction::State::Off;

        this->sig.moderation.modeChanged.invoke(action);
    };

    this->moderationActionHandlers["r9kbeta"] = [this](const auto &data, const auto &roomID) {
        ModeChangedAction action(data, roomID);

        action.mode = ModeChangedAction::Mode::R9K;
        action.state = ModeChangedAction::State::On;

        this->sig.moderation.modeChanged.invoke(action);
    };

    this->moderationActionHandlers["subscribersoff"] = [this](const auto &data,
                                                              const auto &roomID) {
        ModeChangedAction action(data, roomID);

        action.mode = ModeChangedAction::Mode::SubscribersOnly;
        action.state = ModeChangedAction::State::Off;

        this->sig.moderation.modeChanged.invoke(action);
    };

    this->moderationActionHandlers["subscribers"] = [this](const auto &data, const auto &roomID) {
        ModeChangedAction action(data, roomID);

        action.mode = ModeChangedAction::Mode::SubscribersOnly;
        action.state = ModeChangedAction::State::On;

        this->sig.moderation.modeChanged.invoke(action);
    };

    this->moderationActionHandlers["emoteonlyoff"] = [this](const auto &data, const auto &roomID) {
        ModeChangedAction action(data, roomID);

        action.mode = ModeChangedAction::Mode::EmoteOnly;
        action.state = ModeChangedAction::State::Off;

        this->sig.moderation.modeChanged.invoke(action);
    };

    this->moderationActionHandlers["emoteonly"] = [this](const auto &data, const auto &roomID) {
        ModeChangedAction action(data, roomID);

        action.mode = ModeChangedAction::Mode::EmoteOnly;
        action.state = ModeChangedAction::State::On;

        this->sig.moderation.modeChanged.invoke(action);
    };

    this->moderationActionHandlers["unmod"] = [this](const auto &data, const auto &roomID) {
        ModerationStateAction action(data, roomID);

        getTargetUser(data, action.target);
        action.modded = false;

        this->sig.moderation.moderationStateChanged.invoke(action);
    };

    this->moderationActionHandlers["mod"] = [this](const auto &data, const auto &roomID) {
        ModerationStateAction action(data, roomID);

        getTargetUser(data, action.target);
        action.modded = true;

        this->sig.moderation.moderationStateChanged.invoke(action);
    };

    this->moderationActionHandlers["timeout"] = [this](const auto &data, const auto &roomID) {
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

            this->sig.moderation.userBanned.invoke(action);
        } catch (const std::runtime_error &ex) {
            debug::Log("Error parsing moderation action: {}", ex.what());
        }
    };

    this->moderationActionHandlers["ban"] = [this](const auto &data, const auto &roomID) {
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

            this->sig.moderation.userBanned.invoke(action);
        } catch (const std::runtime_error &ex) {
            debug::Log("Error parsing moderation action: {}", ex.what());
        }
    };

    this->moderationActionHandlers["unban"] = [this](const auto &data, const auto &roomID) {
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

            this->sig.moderation.userUnbanned.invoke(action);
        } catch (const std::runtime_error &ex) {
            debug::Log("Error parsing moderation action: {}", ex.what());
        }
    };

    this->moderationActionHandlers["untimeout"] = [this](const auto &data, const auto &roomID) {
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

            this->sig.moderation.userUnbanned.invoke(action);
        } catch (const std::runtime_error &ex) {
            debug::Log("Error parsing moderation action: {}", ex.what());
        }
    };

    this->websocketClient.set_access_channels(websocketpp::log::alevel::all);
    this->websocketClient.clear_access_channels(websocketpp::log::alevel::frame_payload);

    this->websocketClient.init_asio();

    // SSL Handshake
    this->websocketClient.set_tls_init_handler(bind(&PubSub::OnTLSInit, this, ::_1));

    this->websocketClient.set_message_handler(bind(&PubSub::OnMessage, this, ::_1, ::_2));
    this->websocketClient.set_open_handler(bind(&PubSub::OnConnectionOpen, this, ::_1));
    this->websocketClient.set_close_handler(bind(&PubSub::OnConnectionClose, this, ::_1));

    // Add an initial client
    this->AddClient();
}

void PubSub::AddClient()
{
    websocketpp::lib::error_code ec;
    auto con = this->websocketClient.get_connection(TWITCH_PUBSUB_URL, ec);

    if (ec) {
        debug::Log("Unable to establish connection: {}", ec.message());
        return;
    }

    this->websocketClient.connect(con);
}

void PubSub::Start()
{
    this->mainThread.reset(new std::thread(std::bind(&PubSub::RunThread, this)));
}

void PubSub::ListenToWhispers(std::shared_ptr<providers::twitch::TwitchAccount> account)
{
    assert(account != nullptr);

    std::string userID = account->getUserId().toStdString();

    debug::Log("Connection open!");
    websocketpp::lib::error_code ec;

    std::vector<std::string> topics({"whispers." + userID});

    this->Listen(std::move(CreateListenMessage(topics, account)));

    if (ec) {
        debug::Log("Unable to send message to websocket server: {}", ec.message());
        return;
    }
}

void PubSub::UnlistenAllModerationActions()
{
    for (const auto &p : this->clients) {
        const auto &client = p.second;
        client->UnlistenPrefix("chat_moderator_actions.");
    }
}

void PubSub::ListenToChannelModerationActions(
    const QString &channelID, std::shared_ptr<providers::twitch::TwitchAccount> account)
{
    assert(!channelID.isEmpty());
    assert(account != nullptr);
    QString userID = account->getUserId();
    assert(!userID.isEmpty());

    std::string topic(fS("chat_moderator_actions.{}.{}", userID, channelID));

    if (this->isListeningToTopic(topic)) {
        debug::Log("We are already listening to topic {}", topic);
        return;
    }

    debug::Log("Listen to topic {}", topic);

    this->listenToTopic(topic, account);
}

void PubSub::listenToTopic(const std::string &topic,
                           std::shared_ptr<providers::twitch::TwitchAccount> account)
{
    auto message = CreateListenMessage({topic}, account);

    this->Listen(std::move(message));
}

void PubSub::Listen(rapidjson::Document &&msg)
{
    if (this->TryListen(msg)) {
        debug::Log("Successfully listened!");
        return;
    }

    debug::Log("Added to the back of the queue");
    this->requests.emplace_back(std::make_unique<rapidjson::Document>(std::move(msg)));
}

bool PubSub::TryListen(rapidjson::Document &msg)
{
    debug::Log("TryListen with {} clients", this->clients.size());
    for (const auto &p : this->clients) {
        const auto &client = p.second;
        if (client->Listen(msg)) {
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

void PubSub::OnMessage(websocketpp::connection_hdl hdl, WebsocketMessagePtr websocketMessage)
{
    const std::string &payload = websocketMessage->get_payload();

    rapidjson::Document msg;

    rapidjson::ParseResult res = msg.Parse(payload.c_str());

    if (!res) {
        debug::Log("Error parsing message '{}' from PubSub: {}", payload,
                   rapidjson::GetParseError_En(res.Code()));
        return;
    }

    if (!msg.IsObject()) {
        debug::Log("Error parsing message '{}' from PubSub. Root object is not an object", payload);
        return;
    }

    std::string type;

    if (!rj::getSafe(msg, "type", type)) {
        debug::Log("Missing required string member `type` in message root");
        return;
    }

    if (type == "RESPONSE") {
        this->HandleListenResponse(msg);
    } else if (type == "MESSAGE") {
        if (!msg.HasMember("data")) {
            debug::Log("Missing required object member `data` in message root");
            return;
        }

        const auto &data = msg["data"];

        if (!data.IsObject()) {
            debug::Log("Member `data` must be an object");
            return;
        }

        this->HandleMessageResponse(data);
    } else if (type == "PONG") {
        auto clientIt = this->clients.find(hdl);

        // If this assert goes off, there's something wrong with the connection creation/preserving
        // code KKona
        assert(clientIt != this->clients.end());

        auto &client = *clientIt;

        client.second->HandlePong();
    } else {
        debug::Log("Unknown message type: {}", type);
    }
}

void PubSub::OnConnectionOpen(WebsocketHandle hdl)
{
    auto client = std::make_shared<detail::PubSubClient>(this->websocketClient, hdl);

    // We separate the starting from the constructor because we will want to use shared_from_this
    client->Start();

    this->clients.emplace(hdl, client);

    this->connected.invoke();
}

void PubSub::OnConnectionClose(WebsocketHandle hdl)
{
    auto clientIt = this->clients.find(hdl);

    // If this assert goes off, there's something wrong with the connection creation/preserving
    // code KKona
    assert(clientIt != this->clients.end());

    auto &client = clientIt->second;

    client->Stop();

    this->clients.erase(clientIt);

    this->connected.invoke();
}

PubSub::WebsocketContextPtr PubSub::OnTLSInit(websocketpp::connection_hdl hdl)
{
    WebsocketContextPtr ctx(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1));

    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::single_dh_use);
    } catch (const std::exception &e) {
        debug::Log("Exception caught in OnTLSInit: {}", e.what());
    }

    return ctx;
}

void PubSub::HandleListenResponse(const rapidjson::Document &msg)
{
    std::string error;

    if (rj::getSafe(msg, "error", error)) {
        std::string nonce;
        rj::getSafe(msg, "nonce", nonce);
        const auto &xd = sentMessages;
        const auto &payload = sentMessages[nonce];

        if (error.empty()) {
            debug::Log("Successfully listened to nonce {}", nonce);
            // Nothing went wrong
            return;
        }

        debug::Log("PubSub error: {} on nonce {}", error, nonce);
        return;
    }
}

void PubSub::HandleMessageResponse(const rapidjson::Value &outerData)
{
    QString topic;

    if (!rj::getSafe(outerData, "topic", topic)) {
        debug::Log("Missing required string member `topic` in outerData");
        return;
    }

    std::string payload;

    if (!rj::getSafe(outerData, "message", payload)) {
        debug::Log("Expected string message in outerData");
        return;
    }

    rapidjson::Document msg;

    rapidjson::ParseResult res = msg.Parse(payload.c_str());

    if (!res) {
        debug::Log("Error parsing message '{}' from PubSub: {}", payload,
                   rapidjson::GetParseError_En(res.Code()));
        return;
    }

    if (topic.startsWith("whispers.")) {
        std::string whisperType;

        if (!rj::getSafe(msg, "type", whisperType)) {
            debug::Log("Bad whisper data");
            return;
        }

        if (whisperType == "whisper_received") {
            this->sig.whisper.received.invoke(msg);
        } else if (whisperType == "whisper_sent") {
            this->sig.whisper.sent.invoke(msg);
        } else if (whisperType == "thread") {
            // Handle thread?
        } else {
            debug::Log("Invalid whisper type: {}", whisperType);
            assert(false);
            return;
        }
    } else if (topic.startsWith("chat_moderator_actions.")) {
        auto topicParts = topic.split(".");
        assert(topicParts.length() == 3);
        const auto &data = msg["data"];

        std::string moderationAction;

        if (!rj::getSafe(data, "moderation_action", moderationAction)) {
            debug::Log("Missing moderation action in data: {}", Stringify(data));
            return;
        }

        auto handlerIt = this->moderationActionHandlers.find(moderationAction);

        if (handlerIt == this->moderationActionHandlers.end()) {
            debug::Log("No handler found for moderation action {}", moderationAction);
            return;
        }

        // Invoke handler function
        handlerIt->second(data, topicParts[2]);
    } else {
        debug::Log("Unknown topic: {}", topic);
        return;
    }
}

void PubSub::RunThread()
{
    debug::Log("Start pubsub manager thread");
    this->websocketClient.run();
    debug::Log("Done with pubsub manager thread");
}

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
