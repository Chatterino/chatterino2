#include "providers/twitch/pubsubmessages/AutoMod.hpp"

namespace chatterino {

PubSubAutoModQueueMessage::PubSubAutoModQueueMessage(const QJsonObject &root)
    : typeString(root.value("type").toString())
    , data(root.value("data").toObject())
    , status(this->data.value("status").toString())
{
    auto oType = magic_enum::enum_cast<Type>(this->typeString.toStdString());
    if (oType.has_value())
    {
        this->type = oType.value();
    }

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
}

}  // namespace chatterino
