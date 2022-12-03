#include "NotificationPopup.hpp"

#include "common/Channel.hpp"
#include "messages/Message.hpp"
#include "widgets/helper/ChannelView.hpp"

#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>
#include <QVBoxLayout>

namespace chatterino {

NotificationPopup::NotificationPopup()
    : BaseWindow({BaseWindow::Frameless, BaseWindow::DisableLayoutSave})
    , channel_(std::make_shared<Channel>("notifications", Channel::Type::None))

{
    this->channelView_ = new ChannelView(this);

    auto *layout = new QVBoxLayout(this);
    this->setLayout(layout);

    layout->addWidget(this->channelView_);

    this->channelView_->setChannel(this->channel_);
    this->setScaleIndependantSize(300, 150);
}

void NotificationPopup::updatePosition()
{
    Location location = BottomRight;

    const QRect rect = QGuiApplication::primaryScreen()->availableGeometry();

    switch (location)
    {
        case BottomRight: {
            this->move(rect.right() - this->width(),
                       rect.bottom() - this->height());
        }
        break;
    }
}

void NotificationPopup::addMessage(MessagePtr msg)
{
    this->channel_->addMessage(std::move(msg));

    //    QTimer::singleShot(5000, this, [this, msg] { this->channel->remove });
}

}  // namespace chatterino
