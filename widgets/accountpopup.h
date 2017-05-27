#ifndef USERPOPUPWIDGET_H
#define USERPOPUPWIDGET_H

#include <QWidget>

#include <memory>

namespace Ui {
class AccountPopup;
}

namespace chatterino {

class Channel;

namespace widgets {

class AccountPopupWidget : public QWidget
{
    Q_OBJECT
public:
    AccountPopupWidget(std::shared_ptr<Channel> &channel);

    void setName(const QString &name);

private:
    Ui::AccountPopup *_ui;

    std::shared_ptr<Channel> &_channel;
};

}  // namespace widgets
}  // namespace chatterino

#endif  // USERPOPUPWIDGET_H
