#include "LogsPopup.hpp"

#include "IrcMessage"
#include "common/NetworkRequest.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "widgets/helper/ChannelView.hpp"

#include <QDateTime>
#include <QJsonArray>
#include <QMessageBox>
#include <QVBoxLayout>

namespace chatterino {

LogsPopup::LogsPopup()
{
    this->initLayout();
    this->resize(400, 600);
}

void LogsPopup::initLayout()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);

    this->channelView_ = new ChannelView(this);
    layout->addWidget(this->channelView_);

    this->setLayout(layout);
}

void LogsPopup::setInfo(ChannelPtr channel, QString userName)
{
    this->channel_ = channel;
    this->userName_ = userName;
    this->getRoomID();
    this->setWindowTitle(this->userName_ + "'s logs in #" +
                         this->channel_->getName());
    this->getLogviewerLogs();
}

void LogsPopup::setMessages(std::vector<MessagePtr> &messages)
{
    ChannelPtr logsChannel(new Channel("logs", Channel::Type::Misc));

    logsChannel->addMessagesAtStart(messages);
    this->channelView_->setChannel(logsChannel);
}

void LogsPopup::getRoomID()
{
    TwitchChannel *twitchChannel =
        dynamic_cast<TwitchChannel *>(this->channel_.get());
    if (twitchChannel == nullptr) {
        return;
    }

    QString channelName = twitchChannel->getName();

    QString url = QString("https://cbenni.com/api/channel/%1").arg(channelName);

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());

    req.onError([this](int errorCode) {
        this->getOverrustleLogs();
        return true;
    });

    req.onSuccess([this, channelName](auto result) -> Outcome {
        auto data = result.parseJson();
        this->roomID_ = data.value("channel").toObject()["id"].toInt();
        this->getLogviewerLogs();
        return Success;
    });

    req.execute();
}

void LogsPopup::getLogviewerLogs()
{
    TwitchChannel *twitchChannel =
        dynamic_cast<TwitchChannel *>(this->channel_.get());
    if (twitchChannel == nullptr) {
        return;
    }

    QString channelName = twitchChannel->getName();

    auto url = QString("https://cbenni.com/api/logs/%1/?nick=%2&before=500")
                   .arg(channelName, this->userName_);

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());

    req.onError([this](int errorCode) {
        this->getOverrustleLogs();
        return true;
    });

    req.onSuccess([this, channelName](auto result) -> Outcome {
        auto data = result.parseJson();
        std::vector<MessagePtr> messages;

        QJsonValue before = data.value("before");

        for (auto i : before.toArray()) {
            auto messageObject = i.toObject();
            QString message = messageObject.value("text").toString();

            // Hacky way to fix the timestamp
            message.insert(1, "historical=1;");
            message.insert(1, QString("tmi-sent-ts=%10000;")
                                  .arg(messageObject["time"].toInt()));
            message.insert(1, QString("room-id=%1;").arg(this->roomID_));

            MessageParseArgs args;
            auto ircMessage =
                Communi::IrcMessage::fromData(message.toUtf8(), nullptr);
            auto privMsg =
                static_cast<Communi::IrcPrivateMessage *>(ircMessage);
            TwitchMessageBuilder builder(this->channel_.get(), privMsg, args);
            messages.push_back(builder.build());
        };
        this->setMessages(messages);

        return Success;
    });

    req.execute();
}

void LogsPopup::getOverrustleLogs()
{
    TwitchChannel *twitchChannel =
        dynamic_cast<TwitchChannel *>(this->channel_.get());
    if (twitchChannel == nullptr) {
        return;
    }

    QString channelName = twitchChannel->getName();

    QString url =
        QString("https://overrustlelogs.net/api/v1/stalk/%1/%2.json?limit=500")
            .arg(channelName, this->userName_);

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.onError([this, channelName](int errorCode) {
        this->close();
        auto box = new QMessageBox(
            QMessageBox::Information, "Error getting logs",
            "No logs could be found for channel " + channelName);
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->show();
        box->raise();

        return true;
    });

    req.onSuccess([this, channelName](auto result) -> Outcome {
        auto data = result.parseJson();
        std::vector<MessagePtr> messages;
        if (data.contains("lines")) {
            QJsonArray dataMessages = data.value("lines").toArray();
            for (auto i : dataMessages) {
                QJsonObject singleMessage = i.toObject();
                QTime timeStamp = QDateTime::fromSecsSinceEpoch(
                                      singleMessage.value("timestamp").toInt())
                                      .time();

                MessageBuilder builder;
                builder.emplace<TimestampElement>(timeStamp);
                builder.emplace<TextElement>(this->userName_,
                                             MessageElementFlag::Username,
                                             MessageColor::System);
                builder.emplace<TextElement>(
                    singleMessage.value("text").toString(),
                    MessageElementFlag::Text, MessageColor::Text);
                messages.push_back(builder.release());
            }
        }
        this->setMessages(messages);

        return Success;
    });

    req.execute();
}
}  // namespace chatterino
