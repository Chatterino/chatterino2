#include "providers/seventv/SeventvEventApiClient.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QStringView>
#include "common/QLogging.hpp"
#include "providers/twitch/PubSubHelpers.hpp"
#include "singletons/Settings.hpp"
#include "util/DebugCount.hpp"
#include "util/Helpers.hpp"

#include <exception>
#include <magic_enum.hpp>
#include <thread>

namespace chatterino {
namespace {
    const char *typeToString(SeventvEventApiSubscriptionType type)
    {
        switch (type)
        {
            case SeventvEventApiSubscriptionType::UpdateEmoteSet:
                return "emote_set.update";
            case SeventvEventApiSubscriptionType::UpdateUser:
                return "user.update";
            default:
                return "";
        }
    }

    QJsonObject createDataJson(const char *typeName, const QString &condition)
    {
        QJsonObject data;
        data["type"] = typeName;
        {
            QJsonObject conditionObj;
            conditionObj["object_id"] = condition;
            data["condition"] = conditionObj;
        }
        return data;
    }
}  // namespace
SeventvEventApiClient::SeventvEventApiClient(
    eventapi::WebsocketClient &_websocketClient,
    eventapi::WebsocketHandle _handle)
    : websocketClient_(_websocketClient)
    , handle_(_handle)
    , lastHeartbeat_(std::chrono::steady_clock::now())
    , heartbeatInterval_(std::chrono::milliseconds(25000))
{
}

void SeventvEventApiClient::start()
{
    assert(!this->started_);
    this->started_ = true;
    this->lastHeartbeat_ = std::chrono::steady_clock::now();
    this->checkHeartbeat();
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

void SeventvEventApiClient::setHeartbeatInterval(int intervalMs)
{
    qCDebug(chatterinoSeventvEventApi)
        << "Setting expected heartbeat interval to" << intervalMs << "ms";
    this->heartbeatInterval_.store(std::chrono::milliseconds(intervalMs));
}

bool SeventvEventApiClient::subscribe(
    const SeventvEventApiSubscription &subscription)
{
    if (this->subscriptions_.size() >= SeventvEventApiClient::MAX_LISTENS)
    {
        return false;
    }

    const auto *typeName = typeToString(subscription.type);
    QJsonObject root;
    root["op"] = (int)SeventvEventApiOpcode::Subscribe;
    root["d"] = createDataJson(typeName, subscription.condition);
    if (!this->subscriptions_.emplace(subscription).second)
    {
        qCWarning(chatterinoSeventvEventApi)
            << "Tried subscribing to" << typeName
            << "object_id:" << subscription.condition
            << "but we're already subscribed!";
        return true;  // true because the subscription already exists
    }

    qCDebug(chatterinoSeventvEventApi)
        << "Subscribing to" << typeName
        << "object_id:" << subscription.condition;
    DebugCount::increase("EventApi subscriptions");

    this->send(QJsonDocument(root).toJson());

    return true;
}

bool SeventvEventApiClient::unsubscribe(
    const SeventvEventApiSubscription &subscription)
{
    if (this->subscriptions_.erase(subscription) <= 0)
    {
        return false;
    }

    const auto *typeName = typeToString(subscription.type);
    QJsonObject root;
    root["op"] = (int)SeventvEventApiOpcode::Unsubscribe;
    root["d"] = createDataJson(typeName, subscription.condition);

    qCDebug(chatterinoSeventvEventApi)
        << "Unsubscribing from" << typeName
        << "object_id:" << subscription.condition;
    DebugCount::decrease("EventApi subscriptions");

    this->send(QJsonDocument(root).toJson());
    return true;
}

void SeventvEventApiClient::handleHeartbeat()
{
    this->lastHeartbeat_ = std::chrono::steady_clock::now();
}

bool SeventvEventApiClient::isSubscribedToEmoteSet(const QString &emoteSetId)
{
    return std::any_of(
        this->subscriptions_.begin(), this->subscriptions_.end(),
        [emoteSetId](const SeventvEventApiSubscription &listener) {
            return listener.type ==
                       SeventvEventApiSubscriptionType::UpdateEmoteSet &&
                   listener.condition == emoteSetId;
        });
}

std::unordered_set<SeventvEventApiSubscription>
    SeventvEventApiClient::getSubscriptions() const
{
    return this->subscriptions_;
}

void SeventvEventApiClient::checkHeartbeat()
{
    assert(this->started_);
    if ((std::chrono::steady_clock::now() - this->lastHeartbeat_.load()) >
        3 * this->heartbeatInterval_.load())
    {
        qCDebug(chatterinoSeventvEventApi)
            << "Didn't receive a heartbeat in time, disconnecting!";
        this->close("Didn't receive a heartbeat in time");

        return;
    }

    auto self = this->shared_from_this();

    runAfter(this->websocketClient_.get_io_service(),
             this->heartbeatInterval_.load(), [self](auto timer) {
                 if (!self->started_)
                 {
                     return;
                 }

                 self->checkHeartbeat();
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
