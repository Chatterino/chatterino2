#include "LogsPopup.hpp"

#include "IrcMessage"
#include "common/Channel.hpp"
#include "common/NetworkRequest.hpp"
#include "debug/Log.hpp"
#include "providers/twitch/PartialTwitchUser.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "util/PostToThread.hpp"
#include "widgets/helper/ChannelView.hpp"

#include <QDateTime>
#include <QJsonArray>
#include <QMessageBox>
#include <QVBoxLayout>

namespace chatterino {

LogsPopup::LogsPopup()
    : channel_(Channel::getEmpty())
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

void LogsPopup::setChannelName(QString channelName)
{
    this->channelName_ = channelName;
}

void LogsPopup::setChannel(std::shared_ptr<Channel> channel)
{
    this->channel_ = channel;
}

void LogsPopup::setTargetUserName(QString userName)
{
    this->userName_ = userName;
}

void LogsPopup::getLogs()
{
    if (this->channel_ && !this->channel_->isEmpty())
    {
        if (auto twitchChannel =
                dynamic_cast<TwitchChannel *>(this->channel_.get()))
        {
            this->channelName_ = twitchChannel->getName();
            this->getLogviewerLogs(twitchChannel->roomId());

            this->setWindowTitle(this->userName_ + "'s logs in #" +
                                 this->channelName_);
            return;
        }
    }

    if (!this->channelName_.isEmpty())
    {
        PartialTwitchUser::byName(this->channelName_)
            .getId(
                [=](const QString &roomID) { this->getLogviewerLogs(roomID); },
                this);
        return;
    }

    log("Unable to get logs, no channel name or something specified");
}

void LogsPopup::setMessages(std::vector<MessagePtr> &messages)
{
    ChannelPtr logsChannel(new Channel("logs", Channel::Type::Misc));

    logsChannel->addMessagesAtStart(messages);
    this->channelView_->setChannel(logsChannel);
}

void LogsPopup::getLogviewerLogs(const QString &roomID)
{
    auto url = QString("https://cbenni.com/api/logs/%1/?nick=%2&before=500")
                   .arg(this->channelName_, this->userName_);

    NetworkRequest req(url);
    req.setCaller(this);

    req.onError([this](int errorCode) {
        this->getOverrustleLogs();
        return true;
    });

    req.onSuccess([this, roomID](auto result) -> Outcome {
        auto data = result.parseJson();
        std::vector<MessagePtr> messages;

        QJsonValue before = data.value("before");

        for (auto i : before.toArray())
        {
            auto messageObject = i.toObject();
            QString message = messageObject.value("text").toString();

            // Hacky way to fix the timestamp
            message.insert(1, "historical=1;");
            message.insert(1, QString("tmi-sent-ts=%10000;")
                                  .arg(messageObject["time"].toInt()));
            message.insert(1, QString("room-id=%1;").arg(roomID));

            MessageParseArgs args;
            auto ircMessage =
                Communi::IrcMessage::fromData(message.toUtf8(), nullptr);
            auto privMsg =
                static_cast<Communi::IrcPrivateMessage *>(ircMessage);
            TwitchMessageBuilder builder(this->channel_.get(), privMsg, args);
            messages.push_back(builder.build());
        }
        this->setMessages(messages);

        return Success;
    });

    req.execute();
}

void LogsPopup::getOverrustleLogs()
{
    QString url =
        QString("https://overrustlelogs.net/api/v1/stalk/%1/%2.json?limit=500")
            .arg(this->channelName_, this->userName_);

    NetworkRequest req(url);
    req.setCaller(this);
    req.onError([this](int errorCode) {
        auto box = new QMessageBox(
            QMessageBox::Information, "Error getting logs",
            "No logs could be found for channel " + this->channelName_);
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->show();
        box->raise();

        static QSet<int> closeButtons{
            QMessageBox::Ok,
            QMessageBox::Close,
        };
        if (closeButtons.contains(box->exec()))
        {
            this->close();
        }

        return true;
    });

    req.onSuccess([this](auto result) -> Outcome {
        auto data = result.parseJson();
        std::vector<MessagePtr> messages;
        if (data.contains("lines"))
        {
            QJsonArray dataMessages = data.value("lines").toArray();
            for (auto i : dataMessages)
            {
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
