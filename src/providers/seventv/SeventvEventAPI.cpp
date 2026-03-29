// SPDX-FileCopyrightText: 2022 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/seventv/SeventvEventAPI.hpp"

#include "Application.hpp"
#include "providers/liveupdates/BasicPubSubManager.hpp"
#include "providers/seventv/eventapi/Client.hpp"

#include <QJsonArray>

#include <utility>

namespace chatterino {

using namespace seventv;
using namespace seventv::eventapi;
using namespace Qt::StringLiterals;

class SeventvEventAPIPrivate
    : public BasicPubSubManager<SeventvEventAPIPrivate,
                                seventv::eventapi::Client>
{
public:
    SeventvEventAPIPrivate(SeventvEventAPI &parent, QString host,
                           std::chrono::milliseconds defaultHeartbeatInterval);
    ~SeventvEventAPIPrivate() override;
    SeventvEventAPIPrivate(const SeventvEventAPIPrivate &) = delete;
    SeventvEventAPIPrivate(SeventvEventAPIPrivate &&) = delete;
    SeventvEventAPIPrivate &operator=(const SeventvEventAPIPrivate &) = delete;
    SeventvEventAPIPrivate &operator=(SeventvEventAPIPrivate &&) = delete;

    std::shared_ptr<seventv::eventapi::Client> makeClient();
    void checkHeartbeats();

    /** emote-set ids */
    std::unordered_set<QString> subscribedEmoteSets;
    /** user ids */
    std::unordered_set<QString> subscribedUsers;
    /** Twitch channel ids */
    std::unordered_set<QString> subscribedTwitchChannels;

    std::chrono::milliseconds heartbeatInterval;
    QTimer heartbeatTimer;

    SeventvEventAPI &parent;

    friend BasicPubSubManager<SeventvEventAPI, seventv::eventapi::Client>;
    friend SeventvEventAPI;
};

SeventvEventAPIPrivate::SeventvEventAPIPrivate(
    SeventvEventAPI &parent, QString host,
    std::chrono::milliseconds defaultHeartbeatInterval)
    : BasicPubSubManager(std::move(host), u"7TV"_s)
    , heartbeatInterval(defaultHeartbeatInterval)
    , parent(parent)
{
    QObject::connect(&this->heartbeatTimer, &QTimer::timeout, this,
                     &SeventvEventAPIPrivate::checkHeartbeats);
    this->heartbeatTimer.setInterval(this->heartbeatInterval);
    this->heartbeatTimer.setSingleShot(false);
    this->heartbeatTimer.start();
}

SeventvEventAPIPrivate::~SeventvEventAPIPrivate()
{
    this->stop();
}

std::shared_ptr<Client> SeventvEventAPIPrivate::makeClient()
{
    return std::make_shared<Client>(this->parent, this->heartbeatInterval);
}

void SeventvEventAPIPrivate::checkHeartbeats()
{
    auto minInterval = std::chrono::milliseconds::max();
    for (const auto &[id, client] : this->clients())
    {
        client->checkHeartbeat();
        minInterval = std::min(minInterval, client->heartbeatInterval());
    }
    if (minInterval != std::chrono::milliseconds::max())
    {
        this->heartbeatInterval = minInterval;
        this->heartbeatTimer.setInterval(this->heartbeatInterval);
    }
}

SeventvEventAPI::SeventvEventAPI(
    QString host, std::chrono::milliseconds defaultHeartbeatInterval)
    : private_(std::make_unique<SeventvEventAPIPrivate>(
          *this, std::move(host), defaultHeartbeatInterval))
{
}

SeventvEventAPI::~SeventvEventAPI() = default;

void SeventvEventAPI::subscribeUser(const QString &userID,
                                    const QString &emoteSetID)
{
    if (!userID.isEmpty() &&
        this->private_->subscribedUsers.insert(userID).second)
    {
        this->private_->subscribe(
            {ObjectIDCondition{userID}, SubscriptionType::UpdateUser});
    }
    if (!emoteSetID.isEmpty() &&
        this->private_->subscribedEmoteSets.insert(emoteSetID).second)
    {
        this->private_->subscribe(
            {ObjectIDCondition{emoteSetID}, SubscriptionType::UpdateEmoteSet});
    }
}

void SeventvEventAPI::subscribeTwitchChannel(const QString &id)
{
    if (this->private_->subscribedTwitchChannels.insert(id).second)
    {
        this->private_->subscribe({
            ChannelCondition{id},
            SubscriptionType::CreateCosmetic,
        });
        this->private_->subscribe({
            ChannelCondition{id},
            SubscriptionType::CreateEntitlement,
        });
        this->private_->subscribe({
            ChannelCondition{id},
            SubscriptionType::DeleteEntitlement,
        });
    }
}

void SeventvEventAPI::unsubscribeEmoteSet(const QString &id)
{
    if (this->private_->subscribedEmoteSets.erase(id) > 0)
    {
        this->private_->unsubscribe(
            {ObjectIDCondition{id}, SubscriptionType::UpdateEmoteSet});
    }
}

void SeventvEventAPI::unsubscribeUser(const QString &id)
{
    if (this->private_->subscribedUsers.erase(id) > 0)
    {
        this->private_->unsubscribe(
            {ObjectIDCondition{id}, SubscriptionType::UpdateUser});
    }
}

void SeventvEventAPI::unsubscribeTwitchChannel(const QString &id)
{
    if (this->private_->subscribedTwitchChannels.erase(id) > 0)
    {
        this->private_->unsubscribe({
            ChannelCondition{id},
            SubscriptionType::CreateCosmetic,
        });
        this->private_->unsubscribe({
            ChannelCondition{id},
            SubscriptionType::CreateEntitlement,
        });
        this->private_->unsubscribe({
            ChannelCondition{id},
            SubscriptionType::DeleteEntitlement,
        });
    }
}

void SeventvEventAPI::stop()
{
    this->private_->stop();
}

const liveupdates::Diag &SeventvEventAPI::diag() const
{
    return this->private_->diag;
}

}  // namespace chatterino
