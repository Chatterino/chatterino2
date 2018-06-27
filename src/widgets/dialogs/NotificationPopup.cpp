#include "NotificationPopup.hpp"

#include "widgets/helper/ChannelView.hpp"

#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>

namespace chatterino {

NotificationPopup::NotificationPopup()
    : BaseWindow((QWidget *)nullptr, BaseWindow::Frameless)
    , channel(std::make_shared<Channel>("notifications", Channel::None))

{
    this->channelView = new ChannelView(this);

    auto *layout = new QVBoxLayout(this);
    this->setLayout(layout);

    layout->addWidget(this->channelView);

    this->channelView->setChannel(this->channel);
    this->setScaleIndependantSize(300, 150);
}

void NotificationPopup::updatePosition()
{
    Location location = BottomRight;

    QDesktopWidget *desktop = QApplication::desktop();
    const QRect rect = desktop->availableGeometry();

    switch (location) {
        case BottomRight: {
            this->move(rect.right() - this->width(), rect.bottom() - this->height());
        } break;
    }
}

void NotificationPopup::addMessage(chatterino::MessagePtr msg)
{
    this->channel->addMessage(msg);

    //    QTimer::singleShot(5000, this, [this, msg] { this->channel->remove });
}

}  // namespace chatterino
