#include "providers/seventv/SeventvEventApiManager.hpp"

#include "common/QLogging.hpp"
#include "providers/seventv/eventapimessages/SeventvEventApiDispatch.hpp"
#include "providers/seventv/eventapimessages/SeventvEventApiMessage.hpp"
#include "providers/twitch/PubSubHelpers.hpp"
#include "util/DebugCount.hpp"
#include "util/Helpers.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <algorithm>
#include <exception>
#include <thread>

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

namespace chatterino {
SeventvEventApi::SeventvEventApi(const QString &host)
    : host_(host)
{
    this->websocketClient.set_access_channels(websocketpp::log::alevel::all);
    this->websocketClient.clear_access_channels(
        websocketpp::log::alevel::frame_payload |
        websocketpp::log::alevel::frame_header);

    this->websocketClient.init_asio();

    // SSL Handshake
    this->websocketClient.set_tls_init_handler(
        bind(&SeventvEventApi::onTLSInit, this, ::_1));

    this->websocketClient.set_message_handler(
        bind(&SeventvEventApi::onMessage, this, ::_1, ::_2));
    this->websocketClient.set_open_handler(
        bind(&SeventvEventApi::onConnectionOpen, this, ::_1));
    this->websocketClient.set_close_handler(
        bind(&SeventvEventApi::onConnectionClose, this, ::_1));
    this->websocketClient.set_fail_handler(
        bind(&SeventvEventApi::onConnectionFail, this, ::_1));
}

void SeventvEventApi::addClient()
{
    if (this->addingClient_)
    {
        return;
    }

    qCDebug(chatterinoSeventvEventApi) << "Adding an additional client";

    this->addingClient_ = true;

    websocketpp::lib::error_code ec;
    auto con =
        this->websocketClient.get_connection(this->host_.toStdString(), ec);

    if (ec)
    {
        qCDebug(chatterinoSeventvEventApi)
            << "Unable to establish connection:" << ec.message().c_str();
        return;
    }

    this->websocketClient.connect(con);
}

void SeventvEventApi::start()
{
    this->work_ = std::make_shared<boost::asio::io_service::work>(
        this->websocketClient.get_io_service());
    this->mainThread.reset(
        new std::thread(std::bind(&SeventvEventApi::runThread, this)));
}

void SeventvEventApi::stop()
{
    this->stopping_ = true;

    for (const auto &client : this->clients_)
    {
        client.second->close("Shutting down");
    }

    this->work_.reset();

    if (this->mainThread->joinable())
    {
        this->mainThread->join();
    }

    assert(this->clients_.empty());
}

void SeventvEventApi::subscribeUser(const QString &userId,
                                    const QString &emoteSetId)
{
    if (this->subscribedUsers_.insert(userId).second)
    {
        this->subscribe({userId, SeventvEventApiSubscriptionType::UpdateUser});
    }
    if (this->subscribedEmoteSets_.insert(emoteSetId).second)
    {
        this->subscribe(
            {emoteSetId, SeventvEventApiSubscriptionType::UpdateEmoteSet});
    }
}

void SeventvEventApi::unsubscribeEmoteSet(const QString &id)
{
    if (this->subscribedEmoteSets_.erase(id) > 0)
    {
        this->unsubscribe(
            {id, SeventvEventApiSubscriptionType::UpdateEmoteSet});
    }
}

void SeventvEventApi::unsubscribeUser(const QString &id)
{
    if (this->subscribedUsers_.erase(id) > 0)
    {
        this->unsubscribe({id, SeventvEventApiSubscriptionType::UpdateUser});
    }
}

void SeventvEventApi::subscribe(const SeventvEventApiSubscription &subscription)
{
    if (this->trySubscribe(subscription))
    {
        return;
    }

    this->addClient();
    this->pendingSubscriptions_.emplace_back(subscription);
    DebugCount::increase("EventApi subscription backlog");
}

bool SeventvEventApi::trySubscribe(
    const SeventvEventApiSubscription &subscription)
{
    for (auto &client : this->clients_)
    {
        if (client.second->subscribe(subscription))
        {
            return true;
        }
    }
    return false;
}

void SeventvEventApi::unsubscribe(
    const SeventvEventApiSubscription &subscription)
{
    for (auto &client : this->clients_)
    {
        if (client.second->unsubscribe(subscription))
        {
            return;
        }
    }
}

void SeventvEventApi::onMessage(websocketpp::connection_hdl hdl,
                                WebsocketMessagePtr websocketMessage)
{
    const auto &payload =
        QString::fromStdString(websocketMessage->get_payload());

    auto pMessage = parseSeventvEventApiBaseMessage(payload);

    if (!pMessage)
    {
        qCDebug(chatterinoSeventvEventApi)
            << "Unable to parse incoming event-api message: " << payload;
        return;
    }
    auto message = *pMessage;
    switch (message.op)
    {
        case SeventvEventApiOpcode::Hello: {
            auto clientIt = this->clients_.find(hdl);

            // If this assert goes off, there's something wrong with the connection
            // creation/preserving code KKona
            assert(clientIt != this->clients_.end());

            auto &client = *clientIt;

            client.second->setHeartbeatInterval(
                message.data["heartbeat_interval"].toInt());
        }
        break;
        case SeventvEventApiOpcode::Heartbeat: {
            auto clientIt = this->clients_.find(hdl);

            // If this assert goes off, there's something wrong with the connection
            // creation/preserving code KKona
            assert(clientIt != this->clients_.end());

            auto &client = *clientIt;

            client.second->handleHeartbeat();
        }
        break;
        case SeventvEventApiOpcode::Dispatch: {
            auto dispatch = message.toInner<SeventvEventApiDispatch>();
            if (!dispatch)
            {
                qCDebug(chatterinoSeventvEventApi)
                    << "Malformed dispatch" << payload;
                return;
            }
            this->handleDispatch(*dispatch);
        }
        break;
        default: {
            qCDebug(chatterinoSeventvEventApi) << "Unhandled op: " << payload;
        }
        break;
    }
}

void SeventvEventApi::onConnectionOpen(eventapi::WebsocketHandle hdl)
{
    DebugCount::increase("EventApi connections");
    this->addingClient_ = false;

    this->connectBackoff_.reset();

    auto client =
        std::make_shared<SeventvEventApiClient>(this->websocketClient, hdl);

    // We separate the starting from the constructor because we will want to use
    // shared_from_this
    client->start();

    this->clients_.emplace(hdl, client);

    auto pendingSubsToTake = (std::min)(this->pendingSubscriptions_.size(),
                                        SeventvEventApiClient::MAX_LISTENS);

    qCDebug(chatterinoSeventvEventApi)
        << "EventApi connection opened, subscribing to" << pendingSubsToTake
        << "subscriptions!";

    while (pendingSubsToTake > 0 && !this->pendingSubscriptions_.empty())
    {
        const auto last = std::move(this->pendingSubscriptions_.back());
        this->pendingSubscriptions_.pop_back();
        if (!client->subscribe(last))
        {
            qCDebug(chatterinoSeventvEventApi)
                << "Failed to subscribe to" << last.condition << "-"
                << (int)last.type << "on new client.";
            // TODO: should we try to add a new client here?
            return;
        }
        DebugCount::decrease("EventApi subscription backlog");
        pendingSubsToTake--;
    }

    if (!this->pendingSubscriptions_.empty())
    {
        this->addClient();
    }
}

void SeventvEventApi::onConnectionFail(eventapi::WebsocketHandle hdl)
{
    DebugCount::increase("EventApi failed connections");
    if (auto conn = this->websocketClient.get_con_from_hdl(std::move(hdl)))
    {
        qCDebug(chatterinoSeventvEventApi)
            << "EventApi connection attempt failed (error: "
            << conn->get_ec().message().c_str() << ")";
    }
    else
    {
        qCDebug(chatterinoSeventvEventApi)
            << "EventApi connection attempt failed but we can't get the "
               "connection from a handle.";
    }
    this->addingClient_ = false;
    if (!this->pendingSubscriptions_.empty())
    {
        runAfter(this->websocketClient.get_io_service(),
                 this->connectBackoff_.next(), [this](auto timer) {
                     this->addClient();
                 });
    }
}

void SeventvEventApi::onConnectionClose(eventapi::WebsocketHandle hdl)
{
    qCDebug(chatterinoSeventvEventApi) << "Connection closed";

    DebugCount::decrease("EventApi connections");
    auto clientIt = this->clients_.find(hdl);

    // If this assert goes off, there's something wrong with the connection
    // creation/preserving code KKona
    assert(clientIt != this->clients_.end());

    auto client = clientIt->second;

    this->clients_.erase(clientIt);

    client->stop();

    if (!this->stopping_)
    {
        auto subs = client->getSubscriptions();
        for (const auto &sub : subs)
        {
            this->subscribe(sub);
        }
    }
}

SeventvEventApi::WebsocketContextPtr SeventvEventApi::onTLSInit(
    websocketpp::connection_hdl hdl)
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
        qCDebug(chatterinoSeventvEventApi)
            << "Exception caught in OnTLSInit:" << e.what();
    }

    return ctx;
}

void SeventvEventApi::handleDispatch(const SeventvEventApiDispatch &dispatch)
{
    switch (dispatch.type)
    {
        case SeventvEventApiSubscriptionType::UpdateEmoteSet: {
            for (const auto pushed_ : dispatch.body["pushed"].toArray())
            {
                auto pushed = pushed_.toObject();
                if (pushed["key"].toString() != "emotes")
                {
                    continue;
                }
                SeventvEventApiEmoteAddDispatch added(
                    dispatch, pushed["value"].toObject());
                this->signals_.emoteAdded.invoke(added);
            }
            for (const auto updated_ : dispatch.body["updated"].toArray())
            {
                auto updated = updated_.toObject();
                if (updated["key"].toString() != "emotes")
                {
                    continue;
                }
                SeventvEventApiEmoteUpdateDispatch update(dispatch, updated);
                if (update.emoteName != update.oldEmoteName)
                {
                    this->signals_.emoteUpdated.invoke(update);
                }
            }
            for (const auto pulled_ : dispatch.body["pulled"].toArray())
            {
                auto pulled = pulled_.toObject();
                if (pulled["key"].toString() != "emotes")
                {
                    continue;
                }
                SeventvEventApiEmoteRemoveDispatch removed(
                    dispatch, pulled["old_value"].toObject());
                this->signals_.emoteRemoved.invoke(removed);
            }
        }
        break;
        case SeventvEventApiSubscriptionType::UpdateUser: {
            for (const auto updated_ : dispatch.body["updated"].toArray())
            {
                auto updated = updated_.toObject();
                if (updated["key"].toString() != "connections")
                {
                    continue;
                }
                for (const auto value_ : updated["value"].toArray())
                {
                    auto value = value_.toObject();
                    if (value["key"].toString() != "emote_set")
                    {
                        continue;
                    }
                    SeventvEventApiUserConnectionUpdateDispatch update(dispatch,
                                                                       value);
                    this->signals_.userUpdated.invoke(update);
                }
            }
        }
        break;
        default: {
            qCDebug(chatterinoSeventvEventApi)
                << "Unknown subscription type:" << (int)dispatch.type
                << "body:" << dispatch.body;
        }
        break;
    }
}

void SeventvEventApi::runThread()
{
    qCDebug(chatterinoSeventvEventApi) << "Start EventApi manager thread";
    this->websocketClient.run();
    qCDebug(chatterinoSeventvEventApi) << "Done with EventApi manager thread";
}

}  //namespace chatterino
