#pragma once

#include <QColor>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <boost/optional.hpp>

#include <magic_enum.hpp>

namespace chatterino {

// directly translated type from pubsub message, casing is special
enum class PubSubCommunityPointsChannelV1MessageType {
    RewardRedeemed,

    INVALID,
};

}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<
    chatterino::PubSubCommunityPointsChannelV1MessageType>(
    chatterino::PubSubCommunityPointsChannelV1MessageType value) noexcept
{
    switch (value)
    {
        case chatterino::PubSubCommunityPointsChannelV1MessageType::
            RewardRedeemed:
            return "reward-redeemed";
        default:
            return default_tag;
    }
}

namespace chatterino {

enum class PubSubMessageType {
    PONG,
    RESPONSE,
    MESSAGE,

    INVALID,
};

struct PubSubMessageMessage {
    QString nonce;
    QString topic;

    // Unparsed JSON string
    QString message;

    PubSubMessageMessage(QString _nonce, QString _topic, QString _message)
        : nonce(_nonce)
        , topic(_topic)
        , message(_message)
    {
    }
};

struct PubSubMessage {
    QJsonObject object;

    QString nonce;
    QString error;
    QString typeString;
    PubSubMessageType type;

    PubSubMessage(QJsonObject _object)
        : object(_object)
        , nonce(this->object.value("nonce").toString())
        , error(this->object.value("error").toString())
        , typeString(this->object.value("type").toString())
    {
        auto _type = magic_enum::enum_cast<PubSubMessageType>(
            this->typeString.toStdString());
        if (_type.has_value())
        {
            this->type = _type.value();
        }
    }

    boost::optional<PubSubMessageMessage> intoMessage()
    {
        auto dataValue = this->object.value("data");
        if (!dataValue.isObject())
        {
            return boost::none;
        }

        auto data = dataValue.toObject();

        return PubSubMessageMessage{
            this->nonce,
            data.value("topic").toString(),
            data.value("message").toString(),
        };
    }
};

static boost::optional<PubSubMessage> parsePubSubMessage(QString blob)
{
    QJsonDocument jsonDoc(QJsonDocument::fromJson(blob.toUtf8()));

    if (jsonDoc.isNull())
    {
        return boost::none;
    }

    return PubSubMessage(jsonDoc.object());
}

// directly translated type from pubsub message, casing is special
enum class PubSubWhisperMessageType {
    whisper_received,
    whisper_sent,
    thread,

    INVALID,
};

struct PubSubWhisperMessage {
    QString typeString;
    PubSubWhisperMessageType type = PubSubWhisperMessageType::INVALID;

    QString messageID;
    int id;
    QString threadID;
    QString body;
    QString fromUserID;
    QString fromUserLogin;
    QString fromUserDisplayName;
    QColor fromUserColor;

    PubSubWhisperMessage(QJsonObject root)
        : typeString(root.value("type").toString())
    {
        auto _type = magic_enum::enum_cast<PubSubWhisperMessageType>(
            this->typeString.toStdString());
        if (_type.has_value())
        {
            this->type = _type.value();
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
};
//
// directly translated type from pubsub message, casing is special
enum class PubSubChatModeratorActionMessageType {
    moderation_action,
    channel_terms_action,

    INVALID,
};

struct PubSubChatModeratorActionMessage {
    QString typeString;
    PubSubChatModeratorActionMessageType type =
        PubSubChatModeratorActionMessageType::INVALID;

    QJsonObject data;

    PubSubChatModeratorActionMessage(QJsonObject root)
        : typeString(root.value("type").toString())
        , data(root.value("data").toObject())
    {
        auto _type =
            magic_enum::enum_cast<PubSubChatModeratorActionMessageType>(
                this->typeString.toStdString());
        if (_type.has_value())
        {
            this->type = _type.value();
        }
    }
};

struct PubSubCommunityPointsChannelV1Message {
    QString typeString;
    PubSubCommunityPointsChannelV1MessageType type =
        PubSubCommunityPointsChannelV1MessageType::INVALID;

    QJsonObject data;

    PubSubCommunityPointsChannelV1Message(QJsonObject root)
        : typeString(root.value("type").toString())
        , data(root.value("data").toObject())
    {
        auto _type =
            magic_enum::enum_cast<PubSubCommunityPointsChannelV1MessageType>(
                this->typeString.toStdString());
        if (_type.has_value())
        {
            this->type = _type.value();
        }
    }
};
//
// directly translated type from pubsub message, casing is special
enum class PubSubAutomodQueueMessageType {
    automod_caught_message,

    INVALID,
};

struct PubSubAutomodQueueMessage {
    QString typeString;
    PubSubAutomodQueueMessageType type = PubSubAutomodQueueMessageType::INVALID;

    QJsonObject data;

    QString status;

    QString contentCategory;
    int contentLevel;

    QString messageID;
    QString messageText;

    QString senderUserID;
    QString senderUserLogin;
    QString senderUserDisplayName;
    QColor senderUserChatColor;

    PubSubAutomodQueueMessage(QJsonObject root)
        : typeString(root.value("type").toString())
        , data(root.value("data").toObject())
        , status(this->data.value("status").toString())
    {
        auto _type = magic_enum::enum_cast<PubSubAutomodQueueMessageType>(
            this->typeString.toStdString());
        if (_type.has_value())
        {
            this->type = _type.value();
        }

        auto contentClassification =
            data.value("content_classification").toObject();

        this->contentCategory =
            contentClassification.value("category").toString();
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
};

template <class InnerClass>
static boost::optional<InnerClass> parsePubSubDataPayload(QByteArray payload)
{
    QJsonDocument jsonDoc(QJsonDocument::fromJson(payload));

    if (jsonDoc.isNull())
    {
        return boost::none;
    }

    if (!jsonDoc.isObject())
    {
        return boost::none;
    }

    return InnerClass{jsonDoc.object()};
}

}  // namespace chatterino
