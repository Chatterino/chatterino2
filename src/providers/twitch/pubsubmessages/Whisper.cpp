#include "providers/twitch/pubsubmessages/Whisper.hpp"

namespace chatterino {

PubSubWhisperMessage::PubSubWhisperMessage(const QJsonObject &root)
    : typeString(root.value("type").toString())
{
    auto oType = magic_enum::enum_cast<Type>(this->typeString.toStdString());
    if (oType.has_value())
    {
        this->type = oType.value();
    }

    // Parse information from data_object
    auto data = root.value("data_object").toObject();

    this->messageID = data.value("message_id").toString();
    this->id = data.value("id").toInt();
    this->threadID = data.value("thread_id").toString();
    this->body = data.value("body").toString();
    auto fromID = data.value("from_id");
    if (fromID.isString())
    {
        this->fromUserID = fromID.toString();
    }
    else
    {
        this->fromUserID = QString::number(data.value("from_id").toInt());
    }

    auto tags = data.value("tags").toObject();

    this->fromUserLogin = tags.value("login").toString();
    this->fromUserDisplayName = tags.value("display_name").toString();
    this->fromUserColor = QColor(tags.value("color").toString());
}

}  // namespace chatterino
