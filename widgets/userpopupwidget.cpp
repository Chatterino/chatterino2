#include "userpopupwidget.h"
#include "channel.h"
#include "ui_userpopup.h"

#include <QDebug>

namespace chatterino {
namespace widgets {

UserPopupWidget::UserPopupWidget(std::shared_ptr<Channel> &&_channel)
    : QWidget(nullptr)
    , ui(new Ui::UserPopup)
    , channel(std::move(_channel))
{
    this->ui->setupUi(this);

    this->resize(0, 0);

    this->setWindowFlags(Qt::FramelessWindowHint);

    // Close button
    connect(this->ui->btnClose, &QPushButton::clicked, [=]() {
        this->hide();  //
    });

    connect(this->ui->btnPurge, &QPushButton::clicked, [=]() {
        qDebug() << "xD: " << this->channel->getName();
        /*
        this->channel->sendMessage(
            QString(".timeout %1 0").arg(this->ui->lblUsername->text()));
            */
    });
}

void
UserPopupWidget::setName(const QString &name)
{
    this->ui->lblUsername->setText(name);
}

}  // namespace widgets
}  // namespace chatterino
