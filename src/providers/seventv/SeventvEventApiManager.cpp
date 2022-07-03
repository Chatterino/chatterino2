#include "providers/seventv/SeventvEventApiManager.hpp"

#include "common/QLogging.hpp"
#include "providers/seventv/SeventvEventApiMessages.hpp"
#include "providers/twitch/PubSubHelpers.hpp"
#include "util/DebugCount.hpp"
#include "util/Helpers.hpp"

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
    if (this->addingClient)
    {
        return;
    }

    qCDebug(chatterinoSeventvEventApi) << "Adding an additional client";

    this->addingClient = true;

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
    this->work = std::make_shared<boost::asio::io_service::work>(
        this->websocketClient.get_io_service());
    this->mainThread.reset(
        new std::thread(std::bind(&SeventvEventApi::runThread, this)));
}

void SeventvEventApi::stop()
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

void SeventvEventApi::joinChannel(const QString &channel)
{
    if (this->tryJoinChannel(channel))
    {
        return;
    }

    this->addClient();
    this->pendingChannels.emplace_back(channel);
    DebugCount::increase("EventApi channel backlog");
}

void SeventvEventApi::partChannel(const QString &channel)
{
    auto client = std::find_if(
        this->clients.begin(), this->clients.end(), [channel](auto client) {
            return client.second->isJoinedChannel(channel);
        });
    if (client != this->clients.end())
    {
        client->second->part(channel);
    }
}

bool SeventvEventApi::tryJoinChannel(const QString &channel)
{
    return std::any_of(this->clients.begin(), this->clients.end(),
                       [channel](auto client) {
                           return client.second->join(channel);
                       });
}

bool SeventvEventApi::isJoinedChannel(const QString &channel)
{
    return std::any_of(this->clients.begin(), this->clients.end(),
                       [channel](auto client) {
                           return client.second->isJoinedChannel(channel);
                       });
}

void SeventvEventApi::onMessage(websocketpp::connection_hdl hdl,
                                WebsocketMessagePtr websocketMessage)
{
    const auto &payload =
        QString::fromStdString(websocketMessage->get_payload());

    auto pMessage = parseEventApiBaseMessage(payload);

    if (!pMessage)
    {
        qCDebug(chatterinoSeventvEventApi)
            << "Unable to parse incoming event-api message: " << payload;
        return;
    }
    auto message = *pMessage;
    switch (message.action)
    {
        case EventApiMessage::Action::Ping: {
            auto clientIt = this->clients.find(hdl);

            // If this assert goes off, there's something wrong with the connection
            // creation/preserving code KKona
            assert(clientIt != this->clients.end());

            auto &client = *clientIt;

            client.second->handlePing();
        }
        break;
        case EventApiMessage::Action::Update: {
            auto emoteUpdate = message.toInner<EventApiEmoteUpdate>();
            if (!emoteUpdate)
            {
                qCDebug(chatterinoSeventvEventApi)
                    << "Malformed update" << payload;
                return;
            }
            this->handleUpdateAction(*emoteUpdate);
        }
        break;
        case EventApiMessage::Action::Success:
            break;  // we don't get any information on the channel
        case EventApiMessage::Action::Error: {
            qCDebug(chatterinoSeventvEventApi)
                << "Got an error:" << message.json.value("error").toString();
        }
        break;
        case EventApiMessage::Action::INVALID: {
            qCDebug(chatterinoSeventvEventApi)
                << "Unknown message action:" << message.actionString;
        }
        break;
    }
}

void SeventvEventApi::onConnectionOpen(eventapi::WebsocketHandle hdl)
{
    DebugCount::increase("PubSub connections");
    this->addingClient = false;

    this->connectBackoff.reset();

    auto client =
        std::make_shared<SeventvEventApiClient>(this->websocketClient, hdl);

    // We separate the starting from the constructor because we will want to use
    // shared_from_this
    client->start();

    this->clients.emplace(hdl, client);

    auto pendingChannelsToTake = (std::min)(this->pendingChannels.size(),
                                            SeventvEventApiClient::MAX_LISTENS);

    qCDebug(chatterinoSeventvEventApi) << "EventApi connection opened, joining"
                                       << pendingChannelsToTake << "channels!";

    while (pendingChannelsToTake > 0 && !this->pendingChannels.empty())
    {
        const auto last = std::move(this->pendingChannels.back());
        this->pendingChannels.pop_back();
        if (!client->join(last))
        {
            qCDebug(chatterinoSeventvEventApi)
                << "Failed to join " << last << " on new client.";
            // TODO: should we try to add a new client here?
            return;
        }
        DebugCount::decrease("EventApi channel backlog");
        pendingChannelsToTake--;
    }

    if (!this->pendingChannels.empty())
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
    this->addingClient = false;
    if (!this->pendingChannels.empty())
    {
        runAfter(this->websocketClient.get_io_service(),
                 this->connectBackoff.next(), [this](auto timer) {
                     this->addClient();
                 });
    }
}

void SeventvEventApi::onConnectionClose(eventapi::WebsocketHandle hdl)
{
    qCDebug(chatterinoSeventvEventApi) << "Connection closed";

    DebugCount::decrease("EventApi connections");
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
            this->joinChannel(listener.channel);
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

void SeventvEventApi::handleUpdateAction(const EventApiEmoteUpdate &update)
{
    switch (update.action)
    {
        case EventApiEmoteUpdate::Action::Add: {
            this->signals_.emoteAdded.invoke(update);
        }
        break;
        case EventApiEmoteUpdate::Action::Update: {
            this->signals_.emoteUpdated.invoke(update);
        }
        break;
        case EventApiEmoteUpdate::Action::Remove: {
            this->signals_.emoteRemoved.invoke(update);
        }
        break;
        case EventApiEmoteUpdate::Action::INVALID: {
            qCDebug(chatterinoSeventvEventApi)
                << "Got invalid emote update action: " << update.actionString;
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
