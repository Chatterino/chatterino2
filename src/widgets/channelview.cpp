#include "widgets/channelview.hpp"
#include "channelmanager.hpp"
#include "colorscheme.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/message.hpp"
#include "messages/messageref.hpp"
#include "settingsmanager.hpp"
#include "ui_accountpopupform.h"
#include "util/distancebetweenpoints.hpp"
#include "widgets/chatwidget.hpp"
#include "windowmanager.hpp"

#include <QDebug>
#include <QDesktopServices>
#include <QGraphicsBlurEffect>
#include <QPainter>

#include <math.h>
#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>

using namespace chatterino::messages;

namespace chatterino {
namespace widgets {

ChannelView::ChannelView(WindowManager &windowManager, BaseWidget *parent)
    : BaseWidget(parent)
    , windowManager(windowManager)
    , userPopupWidget(std::shared_ptr<twitch::TwitchChannel>())
{
    setLayout(&vbox);
    vbox.addWidget(&web);

    web.load(QUrl("qrc:/chat.html"));
    web.page()->setBackgroundColor(this->colorScheme.ChatBackground);
    web.setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
}

ChannelView::~ChannelView()
{
}

void ChannelView::clearMessages()
{
    this->messages.clear();
}

messages::LimitedQueueSnapshot<SharedMessageRef> ChannelView::getMessagesSnapshot()
{
    return this->messages.getSnapshot();
}

void ChannelView::setChannel(std::shared_ptr<Channel> channel)
{
    if (this->channel) {
        this->detachChannel();
    }
    this->messages.clear();

    // on new message
    this->messageAppendedConnection =
        channel->messageAppended.connect([this](SharedMessage &message) {
            // SharedMessageRef deleted;

            auto command = QString("addMessage('%1','%2'").arg("", "");
            for (const auto &word : message->getWords()) {
                command += ",";
                if (word.isText()) {
                    command += "{type:'text', data:'" + word.getText() + "'}";
                } else {
                    command += "{type:'emote', data:'" + word.getEmoteURL() + "'}";
                }
            }
            command += ");";
            qDebug() << command;
            web.page()->runJavaScript(command);

            /*if (this->messages.appendItem(SharedMessageRef(messageRef), deleted)) {
                qreal value = std::max(0.0, this->getScrollBar().getDesiredValue() - 1);

                this->getScrollBar().setDesiredValue(value, false);
            }*/

            // layoutMessages();
            // update();
        });

    // on message removed
    this->messageRemovedConnection =
        channel->messageRemovedFromStart.connect([](SharedMessage &) {});

    auto snapshot = channel->getMessageSnapshot();

    for (size_t i = 0; i < snapshot.getLength(); i++) {
        SharedMessageRef deleted;

        auto messageRef = new MessageRef(snapshot[i]);

        this->messages.appendItem(SharedMessageRef(messageRef), deleted);
    }

    this->channel = channel;

    this->userPopupWidget.setChannel(channel);
}

void ChannelView::detachChannel()
{
    // on message added
    this->messageAppendedConnection.disconnect();

    // on message removed
    this->messageRemovedConnection.disconnect();
}

}  // namespace widgets
}  // namespace chatterino
