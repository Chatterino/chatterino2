#ifndef USERPOPUPWIDGET_H
#define USERPOPUPWIDGET_H

#include <QWidget>

#include <memory>

namespace Ui {
class UserPopup;
}

namespace chatterino {

class Channel;

namespace widgets {

class UserPopupWidget : public QWidget
{
    Q_OBJECT
public:
    UserPopupWidget(std::shared_ptr<Channel> &&_channel);

    void setName(const QString &name);

private:
    Ui::UserPopup *_ui;

    std::shared_ptr<Channel> _channel;
};

}  // namespace widgets
}  // namespace chatterino

#endif  // USERPOPUPWIDGET_H
