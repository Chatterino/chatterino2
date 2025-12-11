#include "providers/twitch/PubSubManager.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "providers/liveupdates/BasicPubSubManager.hpp"
#include "providers/twitch/PubSubClient.hpp"

#include <QJsonArray>

#include <memory>
#include <utility>

using namespace std::chrono_literals;

namespace chatterino {

class PubSubManagerPrivate
    : public BasicPubSubManager<PubSubManagerPrivate, PubSubClient>
{
public:
    PubSubManagerPrivate(PubSub &parent, QString host,
                         std::chrono::milliseconds heartbeatInterval);
    ~PubSubManagerPrivate() override;
    PubSubManagerPrivate(const PubSubManagerPrivate &) = delete;
    PubSubManagerPrivate(PubSubManagerPrivate &&) = delete;
    PubSubManagerPrivate &operator=(const PubSubManagerPrivate &) = delete;
    PubSubManagerPrivate &operator=(PubSubManagerPrivate &&) = delete;

    std::shared_ptr<PubSubClient> makeClient();
    void checkHeartbeats();

    std::chrono::milliseconds heartbeatInterval;
    QTimer heartbeatTimer;

    PubSub &parent;

    friend BasicPubSubManager<PubSubManagerPrivate, PubSubClient>;
    friend PubSub;
};

PubSubManagerPrivate::PubSubManagerPrivate(
    PubSub &parent, QString host, std::chrono::milliseconds heartbeatInterval)
    : BasicPubSubManager(std::move(host), "PubSub")
    , heartbeatInterval(heartbeatInterval)
    , parent(parent)
{
    QObject::connect(&this->heartbeatTimer, &QTimer::timeout, this,
                     &PubSubManagerPrivate::checkHeartbeats);
    this->heartbeatTimer.setInterval(this->heartbeatInterval);
    this->heartbeatTimer.setSingleShot(false);
    this->heartbeatTimer.start();
}

PubSubManagerPrivate::~PubSubManagerPrivate()
{
    this->stop();
}

void PubSubManagerPrivate::checkHeartbeats()
{
    for (const auto &[id, client] : this->clients())
    {
        client->checkHeartbeat();
    }
}

std::shared_ptr<PubSubClient> PubSubManagerPrivate::makeClient()
{
    return std::make_shared<PubSubClient>(this->parent,
                                          this->heartbeatInterval);
}

PubSub::PubSub(const QString &host, std::chrono::seconds heartbeatInterval)
    : private_(new PubSubManagerPrivate(*this, host, heartbeatInterval))
{
}

PubSub::~PubSub() = default;

const liveupdates::Diag &PubSub::wsDiag() const
{
    return this->private_->diag;
}

void PubSub::stop()
{
    this->private_->stop();
}

void PubSub::listenToChannelPointRewards(const QString &channelID)
{
    static const QString topicFormat("community-points-channel-v1.%1");
    assert(!channelID.isEmpty());

    auto topic = topicFormat.arg(channelID);

    qCDebug(chatterinoPubSub) << "Listen to topic" << topic;
    this->private_->subscribe(TopicData{.topic = std::move(topic)});
}

}  // namespace chatterino
