#include "providers/twitch/PubSubClient.hpp"

#include "common/QLogging.hpp"
#include "providers/twitch/PubSubManager.hpp"
#include "providers/twitch/PubSubMessages.hpp"

namespace chatterino {

using namespace Qt::Literals;

QDebug operator<<(QDebug debug, const TopicData &data)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "TopicData(" << data.topic << ')';

    return debug;
}

PubSubClient::PubSubClient(PubSub &manager,
                           std::chrono::milliseconds heartbeatInterval)
    : BasicPubSubClient(MAX_LISTENS)
    , heartbeatInterval_(heartbeatInterval)
    , manager_(manager)
{
}

void PubSubClient::onOpen()
{
    BasicPubSubClient::onOpen();
    this->isOpen_ = true;
    this->lastHeartbeat_ = std::chrono::steady_clock::now();
}

void PubSubClient::onMessage(const QByteArray &msg)
{
    this->manager_.diag.messagesReceived++;

    auto optMessage = parsePubSubBaseMessage(msg);
    if (!optMessage)
    {
        qCDebug(chatterinoPubSub)
            << "Unable to parse incoming pubsub message" << msg;
        this->manager_.diag.messagesFailedToParse += 1;
        return;
    }

    auto message = *optMessage;

    switch (message.type)
    {
        case PubSubMessage::Type::Pong: {
            this->lastHeartbeat_.store(std::chrono::steady_clock::now());
        }
        break;

        case PubSubMessage::Type::Response: {
            this->handleResponse(message);
        }
        break;

        case PubSubMessage::Type::Message: {
            auto oMessageMessage = message.toInner<PubSubMessageMessage>();
            if (!oMessageMessage)
            {
                qCDebug(chatterinoPubSub) << "Malformed MESSAGE:" << msg;
                return;
            }

            this->handleMessageResponse(*oMessageMessage);
        }
        break;

        case PubSubMessage::Type::INVALID:
        default: {
            qCDebug(chatterinoPubSub)
                << "Unknown message type:" << message.typeString;
        }
        break;
    }
}

void PubSubClient::checkHeartbeat()
{
    if (!this->isOpen_)
    {
        return;
    }

    auto dur = std::chrono::steady_clock::now() - this->lastHeartbeat_.load();
    if (dur > this->heartbeatInterval_ * 1.5)
    {
        qCDebug(chatterinoPubSub) << "Heartbeat timed out";
        this->close();
    }

    this->sendText(R"({"type":"PING"})"_ba);
}

QByteArray PubSubClient::encodeSubscription(const Subscription &subscription)
{
    PubSubListenMessage listen({subscription.topic});
    this->nonces_[listen.nonce] = NonceInfo{
        .isListen = true,
    };
    return listen.toJson();
}

QByteArray PubSubClient::encodeUnsubscription(const Subscription &subscription)
{
    PubSubUnlistenMessage unlisten({subscription.topic});
    this->nonces_[unlisten.nonce] = NonceInfo{
        .isListen = true,
    };
    return unlisten.toJson();
}

void PubSubClient::handleResponse(const PubSubMessage &message)
{
    const bool failed = !message.error.isEmpty();

    if (failed)
    {
        qCDebug(chatterinoPubSub)
            << "Error" << message.error << "on nonce" << message.nonce;
    }

    if (message.nonce.isEmpty())
    {
        // Can't do any specific handling since no nonce was specified
        return;
    }

    auto nonceInfoIt = this->nonces_.find(message.nonce);
    if (nonceInfoIt == this->nonces_.end())
    {
        qCDebug(chatterinoPubSub) << "Unknown nonce:" << message.nonce;
        return;
    }

    if (nonceInfoIt->second.isListen)
    {
        if (failed)
        {
            this->manager_.diag.failedListenResponses++;
        }
        else
        {
            this->manager_.diag.listenResponses++;
        }
    }
    else
    {
        this->manager_.diag.unlistenResponses++;
    }

    this->nonces_.erase(nonceInfoIt);
}

void PubSubClient::handleMessageResponse(const PubSubMessageMessage &message)
{
    if (!message.topic.startsWith("community-points-channel-v1."))
    {
        return;
    }

    auto oInnerMessage =
        message.toInner<PubSubCommunityPointsChannelV1Message>();
    if (!oInnerMessage)
    {
        qCDebug(chatterinoPubSub)
            << "Malformed community-points-channel-v1 message";
        return;
    }

    const auto &innerMessage = *oInnerMessage;

    switch (innerMessage.type)
    {
        case PubSubCommunityPointsChannelV1Message::Type::
            AutomaticRewardRedeemed:
        case PubSubCommunityPointsChannelV1Message::Type::RewardRedeemed: {
            auto redemption = innerMessage.data.value("redemption").toObject();
            this->manager_.pointReward.redeemed.invoke(redemption);
        }
        break;

        case PubSubCommunityPointsChannelV1Message::Type::INVALID:
        default: {
            qCDebug(chatterinoPubSub)
                << "Invalid point event type:" << innerMessage.typeString;
        }
        break;
    }
}

}  // namespace chatterino
