#pragma once

#include "basewindow.hpp"
#include "providers/twitch/twitchchannel.hpp"
#include "util/concurrentmap.hpp"

#include <QPushButton>
#include <QWidget>

#include <memory>

namespace Ui {
class AccountPopup;
}  // namespace Ui

namespace chatterino {

class Channel;

namespace widgets {

class AccountPopupWidget : public BaseWindow
{
    Q_OBJECT
public:
    AccountPopupWidget(ChannelPtr _channel);

    void setName(const QString &name);
    void setChannel(ChannelPtr _channel);

public slots:
    void actuallyRefreshButtons();

signals:
    void refreshButtons();

protected:
    void scaleChangedEvent(float newDpi) override;

private:
    Ui::AccountPopup *ui;

    void getUserId();
    void getUserData();
    void loadAvatar(const QUrl &avatarUrl);

    void updateButtons(QWidget *layout, bool state);
    void timeout(QPushButton *button, int time);
    void sendCommand(QPushButton *button, QString command);

    void refreshLayouts();

    enum class UserType { User, Mod, Owner };

    ChannelPtr channel;

    QPixmap avatar;

    util::ConcurrentMap<QString, QPixmap> avatarMap;

    struct User {
        QString username;
        QString userID;
        UserType userType = UserType::User;

        void refreshUserType(const ChannelPtr &channel, bool loggedInUser);
    };

    User loggedInUser;

    User popupWidgetUser;

    struct {
        bool following = false;
        bool ignoring = false;
    } relationship;

protected:
    void focusOutEvent(QFocusEvent *event) override;
    void showEvent(QShowEvent *event) override;
};

}  // namespace widgets
}  // namespace chatterino
