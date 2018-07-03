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

void LogsPopup::setInfo(ChannelPtr channel, QString username)
{
    this->channel_ = channel;
    this->userName = username;
    this->setWindowTitle("Logs for " + userName + " in " + channel->name + "'s history");
    this->getLogviewerLogs();
}

void LogsPopup::setupView()
{
    ChannelPtr logsChannel(new Channel("logs", Channel::None));
    QStringList output = this->answer.split("<br>", QString::SkipEmptyParts);
    std::vector<MessagePtr> messages;

    for (size_t i = 0; i < output.size(); i++) {
        if (this->usedLogviewer) {
            MessageParseArgs args;
            auto ircMessage = Communi::IrcMessage::fromData(output[i].toUtf8(), nullptr);
            auto privMsg = static_cast<Communi::IrcPrivateMessage *>(ircMessage);
            TwitchMessageBuilder builder(logsChannel.get(), privMsg, args);
            if (!builder.isIgnored()) {
                messages.push_back(builder.build());
            }
        } else {
            MessagePtr message(new Message);
            message->addElement(new TimestampElement(this->rustleTime));
            message->addElement(
                new TextElement(this->userName, MessageElement::Username, MessageColor::System));
            message->addElement(
                new TextElement(output[i], MessageElement::Text, MessageColor::Text));
            messages.push_back(message);
        }
    }
    logsChannel->addMessagesAtStart(messages);
    this->channelView->setChannel(logsChannel);
}

void LogsPopup::getLogviewerLogs()
{
    TwitchChannel *twitchChannel = dynamic_cast<TwitchChannel *>(this->channel_.get());

    QString channelName = twitchChannel->name;
    this->channelName_ = channelName;

    QString url = QString("https://cbenni.com/api/logs/%1/?nick=%2&before=500")
                      .arg(channelName, this->userName);

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());

    req.onError([this](int errorCode) {
        this->getOverrustleLogs();
        return true;
    });

    req.getJSON([this, channelName](QJsonObject &dataCbenni) {
        QJsonValue before = dataCbenni.value("before");
        for (int i = before.toArray().size() - 1; i >= 0; --i) {
            int index = before.toArray().size() - 1 - i;
            QString message = before[index]["text"].toString();
            // Hacky way to fix the timestamp
            message.insert(1, "historical=1;");
            message.insert(1, QString("tmi-sent-ts=%10000;").arg(before[index]["time"].toInt()));
            this->answer += message + "<br>";
        };
        this->setupView();
    });

    req.execute();
}

void LogsPopup::getOverrustleLogs()
{
    this->usedLogviewer = false;

    QString channelName = this->channelName_;

    QString urlRustle("https://overrustlelogs.net/api/v1/stalk/" + channelName + "/" +
                      this->userName + ".json?limit=500");

    NetworkRequest req(urlRustle);
    req.setCaller(QThread::currentThread());
    req.onError([this, channelName](int errorCode) {
        this->hide();
        QMessageBox *box = new QMessageBox(QMessageBox::Information, "Error getting logs",
                                           "No logs could be found for channel " + channelName);
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->show();
        box->raise();
        return true;
    });

    req.getJSON([this, channelName](QJsonObject &rustleData) {
        if (rustleData.contains("lines")) {
            QJsonArray messages = rustleData.value("lines").toArray();
            for (auto i : messages) {
                QJsonObject singleMessage = i.toObject();
                this->rustleTime =
                    QDateTime::fromSecsSinceEpoch(singleMessage.value("timestamp").toInt()).time();
                this->answer += QString("%1<br>").arg(singleMessage.value("text").toString());
            }
        }
        this->setupView();
    });
    req.execute();
}
}  // namespace chatterino
