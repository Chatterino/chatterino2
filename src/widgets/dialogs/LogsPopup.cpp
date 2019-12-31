#include "LogsPopup.hpp"

#include "IrcMessage"
#include "common/Channel.hpp"
#include "common/NetworkRequest.hpp"
#include "messages/Message.hpp"
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
    this->resize(400, 600);
}

void LogsPopup::setChannel(const ChannelPtr &channel)
{
    this->channel_ = channel;
    this->updateWindowTitle();
}

void LogsPopup::setChannelName(const QString &channelName)
{
    this->channelName_ = channelName;
    this->updateWindowTitle();
}

void LogsPopup::setTargetUserName(const QString &userName)
{
    this->userName_ = userName;
    this->updateWindowTitle();
}

void LogsPopup::updateWindowTitle()
{
    this->setWindowTitle(this->userName_ + "'s logs in #" + this->channelName_);
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

    qDebug() << "Unable to get logs, no channel name or something specified";
}

void LogsPopup::setMessages(std::vector<MessagePtr> &messages)
{
    ChannelPtr logsChannel(new Channel("logs", Channel::Type::Misc));

    logsChannel->addMessagesAtStart(messages);
    SearchPopup::setChannel(logsChannel);
}

void LogsPopup::getLogviewerLogs(const QString &roomID)
{
    auto url = QString("https://cbenni.com/api/logs/%1/?nick=%2&before=500")
                   .arg(this->channelName_, this->userName_);

    NetworkRequest(url)
        .caller(this)
        .onError([this](NetworkResult) { this->getOverrustleLogs(); })
        .onSuccess([this, roomID](auto result) -> Outcome {
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
                TwitchMessageBuilder builder(this->channel_.get(), privMsg,
                                             args);
                builder.message().searchText = message;

                messages.push_back(builder.build());
            }

            messages.push_back(
                MessageBuilder(systemMessage,
                               "Logs provided by https://cbenni.com")
                    .release());
            this->setMessages(messages);

            return Success;
        })
        .execute();
}

void LogsPopup::getOverrustleLogs()
{
    QString url =
        QString("https://overrustlelogs.net/api/v1/stalk/%1/%2.json?limit=500")
            .arg(this->channelName_, this->userName_);

    NetworkRequest(url)
        .caller(this)
        .onError([this](NetworkResult) {
            auto box = new QMessageBox(
                QMessageBox::Information, "Error getting logs",
                "No logs could be found for channel " + this->channelName_);
            box->setWindowFlag(Qt::WindowStaysOnTopHint);
            box->setAttribute(Qt::WA_DeleteOnClose);
            box->show();
            box->raise();
            this->close();
            box->exec();
        })
        .onSuccess([this](auto result) -> Outcome {
            auto data = result.parseJson();
            std::vector<MessagePtr> messages;
            if (data.contains("lines"))
            {
                QJsonArray dataMessages = data.value("lines").toArray();
                for (auto i : dataMessages)
                {
                    QJsonObject singleMessage = i.toObject();
                    auto text = singleMessage.value("text").toString();
                    QTime timeStamp =
                        QDateTime::fromSecsSinceEpoch(
                            singleMessage.value("timestamp").toInt())
                            .time();

                    MessageBuilder builder;
                    builder.emplace<TimestampElement>(timeStamp);
                    builder.emplace<TextElement>(this->userName_,
                                                 MessageElementFlag::Username,
                                                 MessageColor::System);
                    builder.emplace<TextElement>(text, MessageElementFlag::Text,
                                                 MessageColor::Text);
                    builder.message().messageText = text;
                    builder.message().displayName = this->userName_;
                    messages.push_back(builder.release());
                }
            }
            messages.push_back(
                MessageBuilder(systemMessage,
                               "Logs provided by https://overrustlelogs.net")
                    .release());
            this->setMessages(messages);

            return Success;
        })
        .execute();
}

}  // namespace chatterino
