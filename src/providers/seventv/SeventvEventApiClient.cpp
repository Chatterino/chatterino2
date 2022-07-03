#include "providers/seventv/SeventvEventApiClient.hpp"

#include "common/QLogging.hpp"
#include "providers/twitch/PubSubHelpers.hpp"
#include "singletons/Settings.hpp"
#include "util/DebugCount.hpp"
#include "util/Helpers.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <exception>
#include <thread>

namespace chatterino {
SeventvEventApiClient::SeventvEventApiClient(
    eventapi::WebsocketClient &_websocketClient,
    eventapi::WebsocketHandle _handle)
    : websocketClient_(_websocketClient)
    , handle_(_handle)
    , lastPing_(std::chrono::steady_clock::now())
{
}

void SeventvEventApiClient::start()
{
    assert(!this->started_);
    this->started_ = true;
    this->lastPing_ = std::chrono::steady_clock::now();
    this->checkPing();
}

void SeventvEventApiClient::stop()
{
    assert(this->started_);
    this->started_ = false;
}

void SeventvEventApiClient::close(const std::string &reason,
                                  websocketpp::close::status::value code)
{
    eventapi::WebsocketErrorCode ec;

    auto conn = this->websocketClient_.get_con_from_hdl(this->handle_, ec);
    if (ec)
    {
        qCDebug(chatterinoSeventvEventApi)
            << "Error getting con:" << ec.message().c_str();
        return;
    }

    conn->close(code, reason, ec);
    if (ec)
    {
        qCDebug(chatterinoSeventvEventApi)
            << "Error closing:" << ec.message().c_str();
        return;
    }
}

bool SeventvEventApiClient::join(const QString &channel)
{
    if (this->channels.size() >= SeventvEventApiClient::MAX_LISTENS)
    {
        return false;
    }

    this->channels.emplace_back(EventApiListener{channel});
    rapidjson::Document doc(rapidjson::kObjectType);
    rj::set(doc, "action", "join");
    rj::set(doc, "payload", channel, doc.GetAllocator());

    qCDebug(chatterinoSeventvEventApi) << "Joining " << channel;
    DebugCount::increase("EventApi channels");

    this->send(rj::stringify(doc).toUtf8());

    return true;
}

void SeventvEventApiClient::part(const QString &channel)
{
    bool found = false;
    for (auto it = this->channels.begin(); it != this->channels.end(); it++)
    {
        if (it->channel == channel)
        {
            this->channels.erase(it);
            found = true;
            break;
        }
    }
    if (!found)
    {
        return;
    }

    rapidjson::Document doc(rapidjson::kObjectType);
    rj::set(doc, "action", "part");
    rj::set(doc, "payload", channel, doc.GetAllocator());

    qCDebug(chatterinoSeventvEventApi) << "Part " << channel;
    DebugCount::increase("EventApi channels");

    this->send(rj::stringify(doc).toUtf8());
}

void SeventvEventApiClient::handlePing()
{
    this->lastPing_ = std::chrono::steady_clock::now();
}

bool SeventvEventApiClient::isJoinedChannel(const QString &channel)
{
    return std::any_of(this->channels.begin(), this->channels.end(),
                       [channel](const EventApiListener &listener) {
                           return listener.channel == channel;
                       });
}

std::vector<EventApiListener> SeventvEventApiClient::getListeners() const
{
    return this->channels;
}

void SeventvEventApiClient::checkPing()
{
    assert(this->started_);
    if ((std::chrono::steady_clock::now() - this->lastPing_.load()) >
        SeventvEventApiClient::CHECK_PING_INTERVAL)
    {
        qCDebug(chatterinoSeventvEventApi)
            << "Didn't receive a ping in time, disconnecting!";
        this->close("Didn't receive a ping in time");

        return;
    }

    auto self = this->shared_from_this();

    runAfter(this->websocketClient_.get_io_service(),
             SeventvEventApiClient::CHECK_PING_INTERVAL, [self](auto timer) {
                 if (!self->started_)
                 {
                     return;
                 }

                 self->checkPing();
             });
}

bool SeventvEventApiClient::send(const char *payload)
{
    eventapi::WebsocketErrorCode ec;
    this->websocketClient_.send(this->handle_, payload,
                                websocketpp::frame::opcode::text, ec);

    if (ec)
    {
        qCDebug(chatterinoSeventvEventApi) << "Error sending message" << payload
                                           << ":" << ec.message().c_str();
        // same todo as in pubsub client
        return false;
    }

    return true;
}
}  // namespace chatterino
