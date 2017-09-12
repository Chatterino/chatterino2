#pragma once
#include "concurrentmap.hpp"

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
    AccountPopupWidget();

    void setName(const QString &name);

private:
    Ui::AccountPopup *_ui;

    void getUserId();
    void getUserData();
    void loadAvatar(const QUrl &avatarUrl);

    QString userID;
    QPixmap avatar;

    ConcurrentMap<QString, QPixmap> avatarMap;

protected:
    virtual void focusOutEvent(QFocusEvent *event) override;
};

}  // namespace widgets
}  // namespace chatterino
