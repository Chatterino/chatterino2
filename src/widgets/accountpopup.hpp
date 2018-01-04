#pragma once

#include "basewidget.hpp"
#include "twitch/twitchchannel.hpp"
#include "util/concurrentmap.hpp"

#include <QPushButton>
#include <QWidget>

#include <memory>

namespace Ui {
class AccountPopup;
}

namespace chatterino {

class Channel;

namespace widgets {

class AccountPopupWidget : public BaseWidget
{
    Q_OBJECT
public:
    AccountPopupWidget(std::shared_ptr<Channel> _channel);

    void setName(const QString &name);
    void setChannel(std::shared_ptr<Channel> _channel);

    void updatePermissions();

protected:
    virtual void dpiMultiplierChanged(float oldDpi, float newDpi) override;

private:
    Ui::AccountPopup *ui;

    void getUserId();
    void getUserData();
    void loadAvatar(const QUrl &avatarUrl);

    void updateButtons(QWidget *layout, bool state);
    void timeout(QPushButton *button, int time);
    void sendCommand(QPushButton *button, QString command);

    enum class permissions { User, Mod, Owner };
    permissions permission;

    std::shared_ptr<Channel> channel;

    QString userID;
    QPixmap avatar;

    util::ConcurrentMap<QString, QPixmap> avatarMap;

protected:
    virtual void focusOutEvent(QFocusEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;
};

}  // namespace widgets
}  // namespace chatterino
