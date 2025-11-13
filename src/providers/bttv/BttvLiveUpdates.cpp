#include "providers/bttv/BttvLiveUpdates.hpp"

#include "liveupdates/BttvLiveUpdateClient.hpp"
#include "providers/liveupdates/BasicPubSubManager.hpp"

#include <QJsonDocument>

#include <unordered_set>
#include <utility>

namespace chatterino {

using namespace Qt::StringLiterals;

class BttvLiveUpdatesPrivate
    : public BasicPubSubManager<BttvLiveUpdatesPrivate, BttvLiveUpdateClient>
{
public:
    BttvLiveUpdatesPrivate(BttvLiveUpdates &parent, QString host);
    ~BttvLiveUpdatesPrivate() override;
    BttvLiveUpdatesPrivate(const BttvLiveUpdatesPrivate &) = delete;
    BttvLiveUpdatesPrivate(const BttvLiveUpdatesPrivate &&) = delete;
    BttvLiveUpdatesPrivate &operator=(const BttvLiveUpdatesPrivate &) = delete;
    BttvLiveUpdatesPrivate &operator=(const BttvLiveUpdatesPrivate &&) = delete;

    std::shared_ptr<BttvLiveUpdateClient> makeClient();

    // Contains all joined Twitch channel-ids
    std::unordered_set<QString> joinedChannels;
    BttvLiveUpdates &parent;

    friend BasicPubSubManager<BttvLiveUpdates, BttvLiveUpdateClient>;
    friend BttvLiveUpdates;
};

BttvLiveUpdatesPrivate::BttvLiveUpdatesPrivate(BttvLiveUpdates &parent,
                                               QString host)
    : BasicPubSubManager(std::move(host), u"BTTV"_s)
    , parent(parent)
{
}

BttvLiveUpdatesPrivate::~BttvLiveUpdatesPrivate()
{
    this->stop();
}

std::shared_ptr<BttvLiveUpdateClient> BttvLiveUpdatesPrivate::makeClient()
{
    return std::make_shared<BttvLiveUpdateClient>(this->parent);
}

BttvLiveUpdates::BttvLiveUpdates(QString host)
    : private_(std::make_unique<BttvLiveUpdatesPrivate>(*this, std::move(host)))
{
}

BttvLiveUpdates::~BttvLiveUpdates() = default;

void BttvLiveUpdates::joinChannel(const QString &channelID,
                                  const QString &userID)
{
    if (this->private_->joinedChannels.insert(channelID).second)
    {
        this->private_->subscribe(
            {BttvLiveUpdateSubscriptionChannel{channelID}});
        this->private_->subscribe({BttvLiveUpdateBroadcastMe{
            .twitchID = channelID, .userID = userID}});
    }
}

void BttvLiveUpdates::partChannel(const QString &id)
{
    if (this->private_->joinedChannels.erase(id) > 0)
    {
        this->private_->unsubscribe({BttvLiveUpdateSubscriptionChannel{id}});
    }
}

void BttvLiveUpdates::stop()
{
    this->private_->stop();
}

const liveupdates::Diag &BttvLiveUpdates::diag() const
{
    return this->private_->diag;
}

}  // namespace chatterino
