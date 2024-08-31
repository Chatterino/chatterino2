#include "providers/bttv/BttvLiveUpdates.hpp"

#include "common/Literals.hpp"

#include <QJsonDocument>

#include <utility>

namespace chatterino {

using namespace chatterino::literals;

BttvLiveUpdates::BttvLiveUpdates(QString host)
    : BasicPubSubManager(std::move(host), u"BTTV"_s)
{
}

BttvLiveUpdates::~BttvLiveUpdates()
{
    this->stop();
}

void BttvLiveUpdates::joinChannel(const QString &channelID,
                                  const QString &userName)
{
    if (this->joinedChannels_.insert(channelID).second)
    {
        this->subscribe({BttvLiveUpdateSubscriptionChannel{channelID}});
        this->subscribe({BttvLiveUpdateBroadcastMe{.twitchID = channelID,
                                                   .userName = userName}});
    }
}

void BttvLiveUpdates::partChannel(const QString &id)
{
    if (this->joinedChannels_.erase(id) > 0)
    {
        this->unsubscribe({BttvLiveUpdateSubscriptionChannel{id}});
    }
}

void BttvLiveUpdates::onMessage(
    websocketpp::connection_hdl /*hdl*/,
    BasicPubSubManager<BttvLiveUpdateSubscription>::WebsocketMessagePtr msg)
{
    const auto &payload = QString::fromStdString(msg->get_payload());
    QJsonDocument jsonDoc(QJsonDocument::fromJson(payload.toUtf8()));

    if (jsonDoc.isNull())
    {
        qCDebug(chatterinoBttv) << "Failed to parse live update JSON";
        return;
    }
    auto json = jsonDoc.object();

    auto eventType = json["name"].toString();
    auto eventData = json["data"].toObject();

    if (eventType == "emote_create")
    {
        auto message = BttvLiveUpdateEmoteUpdateAddMessage(eventData);

        if (!message.validate())
        {
            qCDebug(chatterinoBttv) << "Invalid add message" << json;
            return;
        }

        this->signals_.emoteAdded.invoke(message);
    }
    else if (eventType == "emote_update")
    {
        auto message = BttvLiveUpdateEmoteUpdateAddMessage(eventData);

        if (!message.validate())
        {
            qCDebug(chatterinoBttv) << "Invalid update message" << json;
            return;
        }

        this->signals_.emoteUpdated.invoke(message);
    }
    else if (eventType == "emote_delete")
    {
        auto message = BttvLiveUpdateEmoteRemoveMessage(eventData);

        if (!message.validate())
        {
            qCDebug(chatterinoBttv) << "Invalid deletion message" << json;
            return;
        }

        this->signals_.emoteRemoved.invoke(message);
    }
    else if (eventType == "lookup_user")
    {
        // ignored
    }
    else
    {
        qCDebug(chatterinoBttv) << "Unhandled event:" << json;
    }
}

}  // namespace chatterino
