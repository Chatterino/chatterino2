#pragma once

#include "concurrentmap.hpp"
#include "twitch/twitchchannel.hpp"

#include <QPushButton>
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
    AccountPopupWidget(std::shared_ptr<Channel> channel);

    void setName(const QString &name);
    void setChannel(std::shared_ptr<Channel> channel);

    void updatePermissions();

private:
    Ui::AccountPopup *_ui;

    void getUserId();
    void getUserData();
    void loadAvatar(const QUrl &avatarUrl);

    void updateButtons(QWidget *layout, bool state);
    void timeout(QPushButton *button, int time);
    void sendCommand(QPushButton *button, QString command);

    enum class permissions { User, Mod, Owner };
    permissions permission;

    std::shared_ptr<Channel> _channel;

    QString userID;
    QPixmap avatar;

    ConcurrentMap<QString, QPixmap> avatarMap;

protected:
    virtual void focusOutEvent(QFocusEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;
};

}  // namespace widgets
}  // namespace chatterino
