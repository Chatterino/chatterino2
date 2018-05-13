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
        void reset()
        {
            this->following = -1;
            this->ignoring = -1;
        }

        bool isFollowing() const
        {
            return this->following == 1;
        }

        bool isFollowingSet() const
        {
            return this->following != -1;
        }

        void setFollowing(bool newVal)
        {
            if (newVal) {
                this->following = 1;
            } else {
                this->following = 0;
            }
        }

        bool isIgnoring() const
        {
            return this->ignoring == 1;
        }

        bool isIgnoringSet() const
        {
            return this->ignoring != -1;
        }

        void setIgnoring(bool newVal)
        {
            if (newVal) {
                this->ignoring = 1;
            } else {
                this->ignoring = 0;
            }
        }

    private:
        int following = -1;
        int ignoring = -1;
    } relationship;

protected:
    void focusOutEvent(QFocusEvent *event) override;
    void showEvent(QShowEvent *event) override;
};

}  // namespace widgets
}  // namespace chatterino
