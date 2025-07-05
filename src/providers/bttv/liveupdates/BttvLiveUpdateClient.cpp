#include "providers/bttv/liveupdates/BttvLiveUpdateClient.hpp"

#include "providers/bttv/BttvLiveUpdates.hpp"
#include "providers/bttv/liveupdates/BttvLiveUpdateMessages.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace chatterino {

BttvLiveUpdateClient::BttvLiveUpdateClient(BttvLiveUpdates &manager)
    : BasicPubSubClient<BttvLiveUpdateSubscription>(100)
    , manager(manager)
{
}

void BttvLiveUpdateClient::onMessage(const QByteArray &msg)
{
    QJsonDocument jsonDoc(QJsonDocument::fromJson(msg));

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

        this->manager.signals_.emoteAdded.invoke(message);
    }
    else if (eventType == "emote_update")
    {
        auto message = BttvLiveUpdateEmoteUpdateAddMessage(eventData);

        if (!message.validate())
        {
            qCDebug(chatterinoBttv) << "Invalid update message" << json;
            return;
        }

        this->manager.signals_.emoteUpdated.invoke(message);
    }
    else if (eventType == "emote_delete")
    {
        auto message = BttvLiveUpdateEmoteRemoveMessage(eventData);

        if (!message.validate())
        {
            qCDebug(chatterinoBttv) << "Invalid deletion message" << json;
            return;
        }

        this->manager.signals_.emoteRemoved.invoke(message);
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
