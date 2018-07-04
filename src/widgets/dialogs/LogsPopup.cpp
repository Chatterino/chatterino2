#include "LogsPopup.hpp"

#include "IrcMessage"
#include "common/NetworkRequest.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "widgets/helper/ChannelView.hpp"

#include <QDateTime>
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
    {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setMargin(0);

        {
            this->channelView = new ChannelView(this);
            layout->addWidget(this->channelView);
        }

        this->setLayout(layout);
    }
}

void LogsPopup::setInfo(ChannelPtr channel, QString userName)
{
    this->channel_ = channel;
    this->userName = userName;
    this->setWindowTitle("Logs for " + this->userName + " in " + this->channel_->name +
                         "'s history");
    this->getLogviewerLogs();
}

void LogsPopup::setupView(std::vector<MessagePtr> messages)
{
    ChannelPtr logsChannel(new Channel("logs", Channel::Misc));
    /*
                MessagePtr message(new Message);
                message->addElement(new TimestampElement(this->rustleTime));
                message->addElement(
                    new TextElement(this->userName, MessageElement::Username,
       MessageColor::System)); message->addElement( new TextElement(output[line],
       MessageElement::Text, MessageColor::Text)); messages.push_back(message);*/
    logsChannel->addMessagesAtStart(messages);
    this->channelView->setChannel(logsChannel);
}

void LogsPopup::getLogviewerLogs()
{
    TwitchChannel *twitchChannel = dynamic_cast<TwitchChannel *>(this->channel_.get());
    if (twitchChannel == nullptr) {
        return;
    }

    QString channelName = twitchChannel->name;

    QString url = QString("https://cbenni.com/api/logs/%1/?nick=%2&before=500")
                      .arg(channelName, this->userName);

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());

    req.onError([this](int errorCode) {
        this->getOverrustleLogs();
        return true;
    });

    req.getJSON([this, channelName](QJsonObject &data) {

        std::vector<MessagePtr> messages;
        ChannelPtr logsChannel(new Channel("logs", Channel::None));

        QJsonValue before = data.value("before");

        for (auto i : before.toArray()) {
            auto messageObject = i.toObject();
            QString message = messageObject.value("text").toString();

            // Hacky way to fix the timestamp
            message.insert(1, "historical=1;");
            message.insert(1, QString("tmi-sent-ts=%10000;").arg(messageObject["time"].toInt()));

            MessageParseArgs args;
            auto ircMessage = Communi::IrcMessage::fromData(message.toUtf8(), nullptr);
            auto privMsg = static_cast<Communi::IrcPrivateMessage *>(ircMessage);
            TwitchMessageBuilder builder(logsChannel.get(), privMsg, args);
            if (!builder.isIgnored()) {
                messages.push_back(builder.build());
            }
        };
        this->setupView(messages);
    });

    req.execute();
}

void LogsPopup::getOverrustleLogs()
{
    TwitchChannel *twitchChannel = dynamic_cast<TwitchChannel *>(this->channel_.get());

    QString channelName = twitchChannel->name;

    QString url("https://overrustlelogs.net/api/v1/stalk/" + channelName + "/" + this->userName +
                ".json?limit=500");

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.onError([this, channelName](int errorCode) {
        this->close();
        QMessageBox *box = new QMessageBox(QMessageBox::Information, "Error getting logs",
                                           "No logs could be found for channel " + channelName);
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->show();
        box->raise();
        return true;
    });

    req.getJSON([this, channelName](QJsonObject &data) {
        std::vector<MessagePtr> messages;
        if (data.contains("lines")) {
            QJsonArray dataMessages = data.value("lines").toArray();
            for (auto i : dataMessages) {
                QJsonObject singleMessage = i.toObject();
                QTime timeStamp =
                    QDateTime::fromSecsSinceEpoch(singleMessage.value("timestamp").toInt()).time();

                MessagePtr message(new Message);
                message->addElement(new TimestampElement(timeStamp));
                message->addElement(new TextElement(this->userName, MessageElement::Username,
                                                    MessageColor::System));
                message->addElement(new TextElement(singleMessage.value("text").toString(),
                                                    MessageElement::Text, MessageColor::Text));
                messages.push_back(message);
            }
        }
        this->setupView(messages);
    });
    req.execute();
}
}  // namespace chatterino
