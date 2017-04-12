#include "widgets/userpopupwidget.h"
#include "channel.h"
#include "ui_userpopup.h"

#include <QDebug>

namespace chatterino {
namespace widgets {

UserPopupWidget::UserPopupWidget(std::shared_ptr<Channel> &&channel)
    : QWidget(nullptr)
    , _ui(new Ui::UserPopup)
    , _channel(std::move(channel))
{
    _ui->setupUi(this);

    resize(0, 0);

    setWindowFlags(Qt::FramelessWindowHint);

    // Close button
    connect(_ui->btnClose, &QPushButton::clicked, [=]() {
        hide();  //
    });

    connect(_ui->btnPurge, &QPushButton::clicked, [=]() {
        qDebug() << "xD: " << _channel->getName();
        /*
        _channel->sendMessage(
            QString(".timeout %1 0").arg(_ui->lblUsername->text()));
            */
    });
}

void UserPopupWidget::setName(const QString &name)
{
    _ui->lblUsername->setText(name);
}

}  // namespace widgets
}  // namespace chatterino
