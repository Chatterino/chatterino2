#include "providers/twitch/PubSubManager.hpp"

#include "common/QLogging.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/PubSubHelpers.hpp"
#include "providers/twitch/PubSubMessages.hpp"
#include "util/DebugCount.hpp"
#include "util/Helpers.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <algorithm>
#include <exception>
#include <iostream>
#include <thread>

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

namespace chatterino {

PubSub::PubSub(const QString &host, std::chrono::seconds pingInterval)
    : host_(host)
    , clientOptions_({
          pingInterval,
      })
{
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

        const auto args = data.value("args").toArray();

        if (args.empty())
        {
            qCDebug(chatterinoPubSub)
                << "Missing duration argument in slowmode on";
            return;
        }

        bool ok;

        action.duration = args.at(0).toString().toUInt(&ok, 10);

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

        action.target.id = data.value("target_user_id").toString();

        const auto args = data.value("args").toArray();

        if (args.isEmpty())
        {
            return;
        }

        action.target.login = args[0].toString();

        action.modded = false;

        this->signals_.moderation.moderationStateChanged.invoke(action);
    };

    this->moderationActionHandlers["mod"] = [this](const auto &data,
                                                   const auto &roomID) {
        ModerationStateAction action(data, roomID);
        action.modded = true;

        auto innerType = data.value("type").toString();
        if (innerType == "chat_login_moderation")
        {
            // Don't display the old message type
            return;
        }

        action.target.id = data.value("target_user_id").toString();
        action.target.login = data.value("target_user_login").toString();

        this->signals_.moderation.moderationStateChanged.invoke(action);
    };

    this->moderationActionHandlers["timeout"] = [this](const auto &data,
                                                       const auto &roomID) {
        BanAction action(data, roomID);

        action.source.id = data.value("created_by_user_id").toString();
        action.source.login = data.value("created_by").toString();

        action.target.id = data.value("target_user_id").toString();

        const auto args = data.value("args").toArray();

        if (args.size() < 2)
        {
            return;
        }

        action.target.login = args[0].toString();
        bool ok;
        action.duration = args[1].toString().toUInt(&ok, 10);
        action.reason = args[2].toString();  // May be omitted

        this->signals_.moderation.userBanned.invoke(action);
    };

    this->moderationActionHandlers["delete"] = [this](const auto &data,
                                                      const auto &roomID) {
        DeleteAction action(data, roomID);

        action.source.id = data.value("created_by_user_id").toString();
        action.source.login = data.value("created_by").toString();

        action.target.id = data.value("target_user_id").toString();

        const auto args = data.value("args").toArray();

        if (args.size() < 3)
        {
            return;
        }

        action.target.login = args[0].toString();
        bool ok;
        action.messageText = args[1].toString();
        action.messageId = args[2].toString();

        this->signals_.moderation.messageDeleted.invoke(action);
    };

    this->moderationActionHandlers["ban"] = [this](const auto &data,
                                                   const auto &roomID) {
        BanAction action(data, roomID);

        action.source.id = data.value("created_by_user_id").toString();
        action.source.login = data.value("created_by").toString();

        action.target.id = data.value("target_user_id").toString();

        const auto args = data.value("args").toArray();

        if (args.isEmpty())
        {
            return;
        }

        action.target.login = args[0].toString();
        action.reason = args[1].toString();  // May be omitted

        this->signals_.moderation.userBanned.invoke(action);
    };

    this->moderationActionHandlers["unban"] = [this](const auto &data,
                                                     const auto &roomID) {
        UnbanAction action(data, roomID);

        action.source.id = data.value("created_by_user_id").toString();
        action.source.login = data.value("created_by").toString();

        action.target.id = data.value("target_user_id").toString();

        action.previousState = UnbanAction::Banned;

        const auto args = data.value("args").toArray();

        if (args.isEmpty())
        {
            return;
        }

        action.target.login = args[0].toString();

        this->signals_.moderation.userUnbanned.invoke(action);
    };

    this->moderationActionHandlers["untimeout"] = [this](const auto &data,
                                                         const auto &roomID) {
        UnbanAction action(data, roomID);

        action.source.id = data.value("created_by_user_id").toString();
        action.source.login = data.value("created_by").toString();

        action.target.id = data.value("target_user_id").toString();

        action.previousState = UnbanAction::TimedOut;

        const auto args = data.value("args").toArray();

        if (args.isEmpty())
        {
            return;
        }

        action.target.login = args[0].toString();

        this->signals_.moderation.userUnbanned.invoke(action);
    };

    /*
    // This handler is no longer required as we use the automod-queue topic now
    this->moderationActionHandlers["automod_rejected"] =
        [this](const auto &data, const auto &roomID) {
            AutomodAction action(data, roomID);

            action.source.id = data.value("created_by_user_id").toString();
            action.source.login = data.value("created_by").toString();

            action.target.id = data.value("target_user_id").toString();

            const auto args = data.value("args").toArray();

            if (args.isEmpty())
            {
                return;
            }

            action.msgID = data.value("msg_id").toString();

            if (action.msgID.isEmpty())
            {
                // Missing required msg_id parameter
                return;
            }

            action.target.login = args[0].toString();
            action.message = args[1].toString();  // May be omitted
            action.reason = args[2].toString();   // May be omitted

            this->signals_.moderation.autoModMessageBlocked.invoke(action);
        };
    */

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
            action.source.id = data.value("created_by_user_id").toString();
            action.source.login = data.value("created_by").toString();

            action.type = AutomodUserAction::AddPermitted;
            action.message = data.value("text").toString();
            action.source.login = data.value("requester_login").toString();

            this->signals_.moderation.automodUserMessage.invoke(action);
        };

    this->channelTermsActionHandlers["add_blocked_term"] =
        [this](const auto &data, const auto &roomID) {
            // A term has been added
            AutomodUserAction action(data, roomID);
            action.source.id = data.value("created_by_user_id").toString();
            action.source.login = data.value("created_by").toString();

            action.type = AutomodUserAction::AddBlocked;
            action.message = data.value("text").toString();
            action.source.login = data.value("requester_login").toString();

            this->signals_.moderation.automodUserMessage.invoke(action);
        };

    this->moderationActionHandlers["delete_permitted_term"] =
        [this](const auto &data, const auto &roomID) {
            // This term got deleted
            AutomodUserAction action(data, roomID);
            action.source.id = data.value("created_by_user_id").toString();
            action.source.login = data.value("created_by").toString();

            const auto args = data.value("args").toArray();
            action.type = AutomodUserAction::RemovePermitted;

            if (args.isEmpty())
            {
                return;
            }

            action.message = args[0].toString();

            this->signals_.moderation.automodUserMessage.invoke(action);
        };

    this->channelTermsActionHandlers["delete_permitted_term"] =
        [this](const auto &data, const auto &roomID) {
            // This term got deleted
            AutomodUserAction action(data, roomID);
            action.source.id = data.value("created_by_user_id").toString();
            action.source.login = data.value("created_by").toString();

            action.type = AutomodUserAction::RemovePermitted;
            action.message = data.value("text").toString();
            action.source.login = data.value("requester_login").toString();

            this->signals_.moderation.automodUserMessage.invoke(action);
        };

    this->moderationActionHandlers["delete_blocked_term"] =
        [this](const auto &data, const auto &roomID) {
            // This term got deleted
            AutomodUserAction action(data, roomID);

            action.source.id = data.value("created_by_user_id").toString();
            action.source.login = data.value("created_by").toString();

            const auto args = data.value("args").toArray();
            action.type = AutomodUserAction::RemoveBlocked;

            if (args.isEmpty())
            {
                return;
            }

            action.message = args[0].toString();

            this->signals_.moderation.automodUserMessage.invoke(action);
        };
    this->channelTermsActionHandlers["delete_blocked_term"] =
        [this](const auto &data, const auto &roomID) {
            // This term got deleted
            AutomodUserAction action(data, roomID);

            action.source.id = data.value("created_by_user_id").toString();
            action.source.login = data.value("created_by").toString();

            action.type = AutomodUserAction::RemoveBlocked;
            action.message = data.value("text").toString();
            action.source.login = data.value("requester_login").toString();

            this->signals_.moderation.automodUserMessage.invoke(action);
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
            // qCDebug(chatterinoPubSub) << rj::stringify(data);
        };

    this->moderationActionHandlers["approved_automod_message"] =
        [](const auto &data, const auto &roomID) {
            // This message got approved by a moderator
            // qCDebug(chatterinoPubSub) << rj::stringify(data);
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
    this->websocketClient.set_fail_handler(
        bind(&PubSub::onConnectionFail, this, ::_1));
}

void PubSub::addClient()
{
    if (this->addingClient)
    {
        return;
    }

    qCDebug(chatterinoPubSub) << "Adding an additional client";

    this->addingClient = true;

    websocketpp::lib::error_code ec;
    auto con =
        this->websocketClient.get_connection(this->host_.toStdString(), ec);

    if (ec)
    {
        qCDebug(chatterinoPubSub)
            << "Unable to establish connection:" << ec.message().c_str();
        return;
    }

    this->websocketClient.connect(con);
}

void PubSub::start()
{
    this->work = std::make_shared<boost::asio::io_service::work>(
        this->websocketClient.get_io_service());
    this->mainThread.reset(
        new std::thread(std::bind(&PubSub::runThread, this)));
}

void PubSub::stop()
{
    this->stopping_ = true;

    for (const auto &client : this->clients)
    {
        client.second->close("Shutting down");
    }

    this->work.reset();

    if (this->mainThread->joinable())
    {
        this->mainThread->join();
    }

    assert(this->clients.empty());
}

void PubSub::unlistenAllModerationActions()
{
    for (const auto &p : this->clients)
    {
        const auto &client = p.second;
        if (const auto &[topics, nonce] =
                client->unlistenPrefix("chat_moderator_actions.");
            !topics.empty())
        {
            this->registerNonce(nonce, {
                                           client,
                                           "UNLISTEN",
                                           topics,
                                           topics.size(),
                                       });
        }
    }
}

void PubSub::unlistenAutomod()
{
    for (const auto &p : this->clients)
    {
        const auto &client = p.second;
        if (const auto &[topics, nonce] =
                client->unlistenPrefix("automod-queue.");
            !topics.empty())
        {
            this->registerNonce(nonce, {
                                           client,
                                           "UNLISTEN",
                                           topics,
                                           topics.size(),
                                       });
        }
    }
}

void PubSub::unlistenWhispers()
{
    for (const auto &p : this->clients)
    {
        const auto &client = p.second;
        if (const auto &[topics, nonce] = client->unlistenPrefix("whispers.");
            !topics.empty())
        {
            this->registerNonce(nonce, {
                                           client,
                                           "UNLISTEN",
                                           topics,
                                           topics.size(),
                                       });
        }
    }
}

bool PubSub::listenToWhispers()
{
    if (this->userID_.isEmpty())
    {
        qCDebug(chatterinoPubSub)
            << "Unable to listen to whispers topic, no user logged in";
        return false;
    }

    static const QString topicFormat("whispers.%1");
    auto topic = topicFormat.arg(this->userID_);

    qCDebug(chatterinoPubSub) << "Listen to whispers" << topic;

    this->listenToTopic(topic);

    return true;
}

void PubSub::listenToChannelModerationActions(const QString &channelID)
{
    if (this->userID_.isEmpty())
    {
        qCDebug(chatterinoPubSub) << "Unable to listen to moderation actions "
                                     "topic, no user logged in";
        return;
    }

    static const QString topicFormat("chat_moderator_actions.%1.%2");
    assert(!channelID.isEmpty());

    auto topic = topicFormat.arg(this->userID_, channelID);

    if (this->isListeningToTopic(topic))
    {
        return;
    }

    qCDebug(chatterinoPubSub) << "Listen to topic" << topic;

    this->listenToTopic(topic);
}

void PubSub::listenToAutomod(const QString &channelID)
{
    if (this->userID_.isEmpty())
    {
        qCDebug(chatterinoPubSub)
            << "Unable to listen to automod topic, no user logged in";
        return;
    }

    static const QString topicFormat("automod-queue.%1.%2");
    assert(!channelID.isEmpty());

    auto topic = topicFormat.arg(this->userID_, channelID);

    if (this->isListeningToTopic(topic))
    {
        return;
    }

    qCDebug(chatterinoPubSub) << "Listen to topic" << topic;

    this->listenToTopic(topic);
}

void PubSub::listenToChannelPointRewards(const QString &channelID)
{
    static const QString topicFormat("community-points-channel-v1.%1");
    assert(!channelID.isEmpty());

    auto topic = topicFormat.arg(channelID);

    if (this->isListeningToTopic(topic))
    {
        return;
    }
    qCDebug(chatterinoPubSub) << "Listen to topic" << topic;

    this->listenToTopic(topic);
}

void PubSub::listen(PubSubListenMessage msg)
{
    if (this->tryListen(msg))
    {
        return;
    }

    this->addClient();

    std::copy(msg.topics.begin(), msg.topics.end(),
              std::back_inserter(this->requests));

    DebugCount::increase("PubSub topic backlog", msg.topics.size());
}

bool PubSub::tryListen(PubSubListenMessage msg)
{
    for (const auto &p : this->clients)
    {
        const auto &client = p.second;
        if (auto success = client->listen(msg); success)
        {
            this->registerNonce(msg.nonce, {
                                               client,
                                               "LISTEN",
                                               msg.topics,
                                               msg.topics.size(),
                                           });
            return true;
        }
    }

    return false;
}

void PubSub::registerNonce(QString nonce, NonceInfo info)
{
    this->nonces_[nonce] = std::move(info);
}

boost::optional<PubSub::NonceInfo> PubSub::findNonceInfo(QString nonce)
{
    // TODO: This should also DELETE the nonceinfo from the map
    auto it = this->nonces_.find(nonce);

    if (it == this->nonces_.end())
    {
        return boost::none;
    }

    return it->second;
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
    this->diag.messagesReceived += 1;

    const auto &payload =
        QString::fromStdString(websocketMessage->get_payload());

    auto oMessage = parsePubSubBaseMessage(payload);

    if (!oMessage)
    {
        qCDebug(chatterinoPubSub)
            << "Unable to parse incoming pubsub message" << payload;
        this->diag.messagesFailedToParse += 1;
        return;
    }

    auto message = *oMessage;

    switch (message.type)
    {
        case PubSubMessage::Type::Pong: {
            auto clientIt = this->clients.find(hdl);

            // If this assert goes off, there's something wrong with the connection
            // creation/preserving code KKona
            assert(clientIt != this->clients.end());

            auto &client = *clientIt;

            client.second->handlePong();
        }
        break;

        case PubSubMessage::Type::Response: {
            this->handleResponse(message);
        }
        break;

        case PubSubMessage::Type::Message: {
            auto oMessageMessage = message.toInner<PubSubMessageMessage>();
            if (!oMessageMessage)
            {
                qCDebug(chatterinoPubSub) << "Malformed MESSAGE:" << payload;
                return;
            }

            this->handleMessageResponse(*oMessageMessage);
        }
        break;

        case PubSubMessage::Type::INVALID:
        default: {
            qCDebug(chatterinoPubSub)
                << "Unknown message type:" << message.typeString;
        }
        break;
    }
}

void PubSub::onConnectionOpen(WebsocketHandle hdl)
{
    this->diag.connectionsOpened += 1;

    DebugCount::increase("PubSub connections");
    this->addingClient = false;

    this->connectBackoff.reset();

    auto client = std::make_shared<PubSubClient>(this->websocketClient, hdl,
                                                 this->clientOptions_);

    // We separate the starting from the constructor because we will want to use
    // shared_from_this
    client->start();

    this->clients.emplace(hdl, client);

    qCDebug(chatterinoPubSub) << "PubSub connection opened!";

    const auto topicsToTake =
        (std::min)(this->requests.size(), PubSubClient::MAX_LISTENS);

    std::vector<QString> newTopics(
        std::make_move_iterator(this->requests.begin()),
        std::make_move_iterator(this->requests.begin() + topicsToTake));

    this->requests.erase(this->requests.begin(),
                         this->requests.begin() + topicsToTake);

    PubSubListenMessage msg(newTopics);
    msg.setToken(this->token_);

    if (auto success = client->listen(msg); !success)
    {
        qCWarning(chatterinoPubSub) << "Failed to listen to " << topicsToTake
                                    << "new topics on new client";
        return;
    }
    DebugCount::decrease("PubSub topic backlog", msg.topics.size());

    this->registerNonce(msg.nonce, {
                                       client,
                                       "LISTEN",
                                       msg.topics,
                                       topicsToTake,
                                   });

    if (!this->requests.empty())
    {
        this->addClient();
    }
}

void PubSub::onConnectionFail(WebsocketHandle hdl)
{
    this->diag.connectionsFailed += 1;

    DebugCount::increase("PubSub failed connections");
    if (auto conn = this->websocketClient.get_con_from_hdl(std::move(hdl)))
    {
        qCDebug(chatterinoPubSub) << "PubSub connection attempt failed (error: "
                                  << conn->get_ec().message().c_str() << ")";
    }
    else
    {
        qCDebug(chatterinoPubSub)
            << "PubSub connection attempt failed but we can't "
               "get the connection from a handle.";
    }

    this->addingClient = false;
    if (!this->requests.empty())
    {
        runAfter(this->websocketClient.get_io_service(),
                 this->connectBackoff.next(), [this](auto timer) {
                     this->addClient();  //
                 });
    }
}

void PubSub::onConnectionClose(WebsocketHandle hdl)
{
    qCDebug(chatterinoPubSub) << "Connection closed";
    this->diag.connectionsClosed += 1;

    DebugCount::decrease("PubSub connections");
    auto clientIt = this->clients.find(hdl);

    // If this assert goes off, there's something wrong with the connection
    // creation/preserving code KKona
    assert(clientIt != this->clients.end());

    auto client = clientIt->second;

    this->clients.erase(clientIt);

    client->stop();

    if (!this->stopping_)
    {
        auto clientListeners = client->getListeners();
        for (const auto &listener : clientListeners)
        {
            this->listenToTopic(listener.topic);
        }
    }
}

PubSub::WebsocketContextPtr PubSub::onTLSInit(websocketpp::connection_hdl hdl)
{
    WebsocketContextPtr ctx(
        new boost::asio::ssl::context(boost::asio::ssl::context::tlsv12));

    try
    {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::single_dh_use);
    }
    catch (const std::exception &e)
    {
        qCDebug(chatterinoPubSub)
            << "Exception caught in OnTLSInit:" << e.what();
    }

    return ctx;
}

void PubSub::handleResponse(const PubSubMessage &message)
{
    const bool failed = !message.error.isEmpty();

    if (failed)
    {
        qCDebug(chatterinoPubSub)
            << "Error" << message.error << "on nonce" << message.nonce;
    }

    if (message.nonce.isEmpty())
    {
        // Can't do any specific handling since no nonce was specified
        return;
    }

    if (auto oInfo = this->findNonceInfo(message.nonce); oInfo)
    {
        const auto info = *oInfo;
        auto client = info.client.lock();
        if (!client)
        {
            qCDebug(chatterinoPubSub) << "Client associated with nonce"
                                      << message.nonce << "is no longer alive";
            return;
        }
        if (info.messageType == "LISTEN")
        {
            client->handleListenResponse(message);
            this->handleListenResponse(info, failed);
        }
        else if (info.messageType == "UNLISTEN")
        {
            client->handleUnlistenResponse(message);
            this->handleUnlistenResponse(info, failed);
        }
        else
        {
            qCDebug(chatterinoPubSub)
                << "Unhandled nonce message type" << info.messageType;
        }

        return;
    }

    qCDebug(chatterinoPubSub) << "Response on unused" << message.nonce
                              << "client/topic listener mismatch?";
}

void PubSub::handleListenResponse(const NonceInfo &info, bool failed)
{
    DebugCount::decrease("PubSub topic pending listens", info.topicCount);
    if (failed)
    {
        this->diag.failedListenResponses++;
        DebugCount::increase("PubSub topic failed listens", info.topicCount);
    }
    else
    {
        this->diag.listenResponses++;
        DebugCount::increase("PubSub topic listening", info.topicCount);
    }
}

void PubSub::handleUnlistenResponse(const NonceInfo &info, bool failed)
{
    this->diag.unlistenResponses++;
    DebugCount::decrease("PubSub topic pending unlistens", info.topicCount);
    if (failed)
    {
        qCDebug(chatterinoPubSub) << "Failed unlistening to" << info.topics;
        DebugCount::increase("PubSub topic failed unlistens", info.topicCount);
    }
    else
    {
        qCDebug(chatterinoPubSub) << "Successful unlistened to" << info.topics;
        DebugCount::decrease("PubSub topic listening", info.topicCount);
    }
}

void PubSub::handleMessageResponse(const PubSubMessageMessage &message)
{
    QString topic = message.topic;

    if (topic.startsWith("whispers."))
    {
        auto oInnerMessage = message.toInner<PubSubWhisperMessage>();
        if (!oInnerMessage)
        {
            return;
        }
        auto whisperMessage = *oInnerMessage;

        switch (whisperMessage.type)
        {
            case PubSubWhisperMessage::Type::WhisperReceived: {
                this->signals_.whisper.received.invoke(whisperMessage);
            }
            break;
            case PubSubWhisperMessage::Type::WhisperSent: {
                this->signals_.whisper.sent.invoke(whisperMessage);
            }
            break;
            case PubSubWhisperMessage::Type::Thread: {
                // Handle thread?
            }
            break;

            case PubSubWhisperMessage::Type::INVALID:
            default: {
                qCDebug(chatterinoPubSub)
                    << "Invalid whisper type:" << whisperMessage.typeString;
            }
            break;
        }
    }
    else if (topic.startsWith("chat_moderator_actions."))
    {
        auto oInnerMessage =
            message.toInner<PubSubChatModeratorActionMessage>();
        if (!oInnerMessage)
        {
            return;
        }

        auto innerMessage = *oInnerMessage;
        auto topicParts = topic.split(".");
        assert(topicParts.length() == 3);

        // Channel ID where the moderator actions are coming from
        auto channelID = topicParts[2];

        switch (innerMessage.type)
        {
            case PubSubChatModeratorActionMessage::Type::ModerationAction: {
                QString moderationAction =
                    innerMessage.data.value("moderation_action").toString();

                auto handlerIt =
                    this->moderationActionHandlers.find(moderationAction);

                if (handlerIt == this->moderationActionHandlers.end())
                {
                    qCDebug(chatterinoPubSub)
                        << "No handler found for moderation action"
                        << moderationAction;
                    return;
                }
                // Invoke handler function
                handlerIt->second(innerMessage.data, channelID);
            }
            break;
            case PubSubChatModeratorActionMessage::Type::ChannelTermsAction: {
                QString channelTermsAction =
                    innerMessage.data.value("type").toString();

                auto handlerIt =
                    this->channelTermsActionHandlers.find(channelTermsAction);

                if (handlerIt == this->channelTermsActionHandlers.end())
                {
                    qCDebug(chatterinoPubSub)
                        << "No handler found for channel terms action"
                        << channelTermsAction;
                    return;
                }
                // Invoke handler function
                handlerIt->second(innerMessage.data, channelID);
            }
            break;

            case PubSubChatModeratorActionMessage::Type::INVALID:
            default: {
                qCDebug(chatterinoPubSub)
                    << "Invalid whisper type:" << innerMessage.typeString;
            }
            break;
        }
    }
    else if (topic.startsWith("community-points-channel-v1."))
    {
        auto oInnerMessage =
            message.toInner<PubSubCommunityPointsChannelV1Message>();
        if (!oInnerMessage)
        {
            return;
        }

        auto innerMessage = *oInnerMessage;

        switch (innerMessage.type)
        {
            case PubSubCommunityPointsChannelV1Message::Type::RewardRedeemed: {
                auto redemption =
                    innerMessage.data.value("redemption").toObject();
                this->signals_.pointReward.redeemed.invoke(redemption);
            }
            break;

            case PubSubCommunityPointsChannelV1Message::Type::INVALID:
            default: {
                qCDebug(chatterinoPubSub)
                    << "Invalid point event type:" << innerMessage.typeString;
            }
            break;
        }
    }
    else if (topic.startsWith("automod-queue."))
    {
        auto oInnerMessage = message.toInner<PubSubAutoModQueueMessage>();
        if (!oInnerMessage)
        {
            return;
        }

        auto innerMessage = *oInnerMessage;

        auto topicParts = topic.split(".");
        assert(topicParts.length() == 3);

        // Channel ID where the moderator actions are coming from
        auto channelID = topicParts[2];

        this->signals_.moderation.autoModMessageCaught.invoke(innerMessage,
                                                              channelID);
    }
    else
    {
        qCDebug(chatterinoPubSub) << "Unknown topic:" << topic;
        return;
    }
}

void PubSub::runThread()
{
    qCDebug(chatterinoPubSub) << "Start pubsub manager thread";
    this->websocketClient.run();
    qCDebug(chatterinoPubSub) << "Done with pubsub manager thread";
}

void PubSub::listenToTopic(const QString &topic)
{
    PubSubListenMessage msg({topic});
    msg.setToken(this->token_);

    this->listen(std::move(msg));
}

}  // namespace chatterino
