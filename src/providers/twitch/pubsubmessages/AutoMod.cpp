#include "providers/twitch/pubsubmessages/AutoMod.hpp"

#include "util/QMagicEnum.hpp"

#include <QJsonArray>

namespace chatterino {

PubSubAutoModQueueMessage::PubSubAutoModQueueMessage(const QJsonObject &root)
    : typeString(root.value("type").toString())
    , data(root.value("data").toObject())
    , status(this->data.value("status").toString())
{
    auto oType = qmagicenum::enumCast<Type>(this->typeString);
    if (oType.has_value())
    {
        this->type = oType.value();
    }

    this->reason =
        qmagicenum::enumCast<Reason>(data.value("reason_code").toString())
            .value_or(Reason::INVALID);

    auto contentClassification =
        data.value("content_classification").toObject();

    this->contentCategory = contentClassification.value("category").toString();
    this->contentLevel = contentClassification.value("level").toInt();

    auto message = data.value("message").toObject();

    this->messageID = message.value("id").toString();

    auto messageContent = message.value("content").toObject();

    this->messageText = messageContent.value("text").toString();

    auto messageSender = message.value("sender").toObject();

    this->senderUserID = messageSender.value("user_id").toString();
    this->senderUserLogin = messageSender.value("login").toString();
    this->senderUserDisplayName =
        messageSender.value("display_name").toString();
    this->senderUserChatColor =
        QColor(messageSender.value("chat_color").toString());

    if (this->reason == Reason::BlockedTerm)
    {
        // Attempt to read the blocked term(s) that caused this message to be blocked
        const auto caughtMessageReason =
            data.value("caught_message_reason").toObject();
        const auto blockedTermFailure =
            caughtMessageReason.value("blocked_term_failure").toObject();
        const auto termsFound =
            blockedTermFailure.value("terms_found").toArray();

        for (const auto &termValue : termsFound)
        {
            const auto term = termValue.toObject();
            const auto termText = term.value("text").toString();
            if (!termText.isEmpty())
            {
                this->blockedTermsFound.insert(termText);
            }
        }
    }
}

}  // namespace chatterino
