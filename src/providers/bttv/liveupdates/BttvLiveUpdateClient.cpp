#include "providers/bttv/liveupdates/BttvLiveUpdateClient.hpp"

#include "Application.hpp"
#include "providers/bttv/BttvBadges.hpp"
#include "providers/bttv/BttvLiveUpdates.hpp"
#include "providers/bttv/liveupdates/BttvLiveUpdateMessages.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringBuilder>

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
        auto message = BttvLiveUpdateUserUpdateMessage(eventData);
        if (!message.validate())
        {
            qCDebug(chatterinoBttv) << "Invalid user update message" << json;
            return;
        }

        if (!message.hasBadge())
        {
            return;
        }

        auto *app = tryGetApp();
        if (app)
        {
            auto *bttvBadges = app->getBttvBadges();
            auto badgeID = bttvBadges->registerBadge(message.badgeObject);
            bttvBadges->assignBadgeToUser(badgeID, UserId{message.userID});
        }
    }
    else
    {
        qCDebug(chatterinoBttv) << "Unhandled event:" << json;
    }
}

void BttvLiveUpdateClient::broadcastMe(const QString &channelID,
                                       const QString &userID)
{
    QJsonObject obj{
        {"name", "broadcast_me"},
        {"data",
         QJsonObject{
             {"provider", "twitch"},
             {"providerId", userID},
             {"channel", QString(u"twitch:" % channelID)},
         }},
    };
    QJsonDocument doc(obj);

    this->sendText(doc.toJson(QJsonDocument::Compact));
}

}  // namespace chatterino
