#include "widgets/channelview.hpp"
#include "channelmanager.hpp"
#include "colorscheme.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/message.hpp"
#include "settingsmanager.hpp"
#include "ui_accountpopupform.h"
#include "util/distancebetweenpoints.hpp"
#include "widgets/chatwidget.hpp"
#include "windowmanager.hpp"

#include <QColor>
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
    web.page()->runJavaScript("clearMessages()");
}

void ChannelView::addMessage(SharedMessage &message)
{
    auto command = QString("addMessage(");
    for (int i = 0; i < message->getWords().size(); i++) {
        auto word = message->getWords()[i];
        if (word.isText()) {
            if (i != 0)
                command += ",";
            command += QString("{type:'text', data:'%1', color: '%2'}")
                           .arg(word.getText(), word.getColor().getColor(this->colorScheme).name());
        } else {
            if (word.getType() == messages::Word::Type::Badges) {
                // command += "{type:'emote', data:'" + word.+ "'}";
            } else if (word.getType() | messages::Word::Type::EmoteImages) {
                if (i != 0)
                    command += ",";
                command += "{type:'emote', data:'" + word.getImageURL() + "'}";
            }
        }
    }
    command += ");";
    qDebug() << command;
    web.page()->runJavaScript(command);
}

void ChannelView::setChannel(std::shared_ptr<Channel> channel)
{
    if (this->channel) {
        this->detachChannel();
    }
    this->clearMessages();

    // on new message
    this->messageAppendedConnection = channel->messageAppended.connect(boost::bind(
        &ChannelView::addMessage, this, _1));  // Has to use boost::bind for some reason.

    this->channel = channel;

    this->userPopupWidget.setChannel(channel);
}

void ChannelView::detachChannel()
{
    // on message added
    this->messageAppendedConnection.disconnect();
}

}  // namespace widgets
}  // namespace chatterino
