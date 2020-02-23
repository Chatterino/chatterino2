#include "NotificationPopup.hpp"

#include "common/Channel.hpp"
#include "messages/Message.hpp"
#include "widgets/helper/ChannelView.hpp"

#include <QApplication>
#include <QDesktopWidget>
#include <QLabel>
#include <QScreen>

namespace chatterino {

NotificationPopup::NotificationPopup()
    : BaseWindow({BaseWindow::Frameless, BaseWindow::TopMost})
{
    auto *layout = new QVBoxLayout(this);
    this->setLayout(layout);

    this->setScaleIndependantSize(360, 120);
}

void NotificationPopup::updatePosition()
{
    Location location = BottomRight;

    QDesktopWidget *desktop = QApplication::desktop();
    const QRect rect = desktop->availableGeometry();

    switch (location)
    {
        case BottomRight:
        {
            this->move(rect.right() - this->width(),
                       rect.bottom() - this->height());
        }
        break;
    }
}

void NotificationPopup::setImageAndText(const QPixmap &image,
                                        const QString &text,
                                        const QString &bottomText)
{
    // TODO: make it look better
    delete this->layout();
    auto *layout = new QHBoxLayout(this);

    auto *imageLabel = new QLabel(this);
    imageLabel->setPixmap(image);
    imageLabel->setScaledContents(true);
    imageLabel->setMinimumSize(1, 1);
    imageLabel->setSizePolicy(QSizePolicy::MinimumExpanding,
                              QSizePolicy::Minimum);
    layout->addWidget(imageLabel, 1);

    auto *vbox = new QVBoxLayout(this);
    layout->addLayout(vbox, 2);

    auto *textLabel = new QLabel(this);
    textLabel->setText(text);
    vbox->addWidget(textLabel);

    auto *bottomTextLabel = new QLabel(this);
    bottomTextLabel->setText(bottomText);
    vbox->addWidget(bottomTextLabel);

    this->setLayout(layout);
}

void NotificationPopup::mousePressEvent(QMouseEvent *event)
{
    mouseDown.invoke(event);
    BaseWindow::mousePressEvent(event);
}

}  // namespace chatterino
