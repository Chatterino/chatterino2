#pragma once

#include "widgets/BaseWindow.hpp"
#include "widgets/helper/ChannelView.hpp"

#include <pajlada/signals/signal.hpp>

class QCheckBox;

namespace chatterino {

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;
class Label;

class UserInfoPopup final : public BaseWindow
{
    Q_OBJECT

public:
    UserInfoPopup(bool closeAutomatically);
    ~UserInfoPopup();

    void setData(const QString &name, const ChannelPtr &channel);

protected:
    virtual void themeChangedEvent() override;
    virtual void scaleChangedEvent(float scale) override;

private:
    void installEvents();
    void updateUserData();
    void updateLatestMessages();

    void loadAvatar(const QUrl &url);
    bool isMod_;
    bool isBroadcaster_;

    QString userName_;
    QString userId_;
    ChannelPtr channel_;

    pajlada::Signals::NoArgSignal userStateChanged_;

    // replace with ScopedConnection once https://github.com/pajlada/signals/pull/10 gets merged
    pajlada::Signals::Connection refreshConnection_;

    std::shared_ptr<bool> hack_;

    struct {
        Button *avatarButton = nullptr;

        Label *nameLabel = nullptr;
        Label *viewCountLabel = nullptr;
        Label *followerCountLabel = nullptr;
        Label *createdDateLabel = nullptr;
        Label *userIDLabel = nullptr;

        QCheckBox *follow = nullptr;
        QCheckBox *ignore = nullptr;
        QCheckBox *ignoreHighlights = nullptr;

        Label *noMessagesLabel = nullptr;
        ChannelView *latestMessages = nullptr;
        QPushButton *refreshButton = nullptr;
    } ui_;

    class TimeoutWidget : public BaseWidget
    {
    public:
        enum Action { Ban, Unban, Timeout };

        TimeoutWidget();

        pajlada::Signals::Signal<std::pair<Action, int>> buttonClicked;

    protected:
        void paintEvent(QPaintEvent *event) override;
    };
};

}  // namespace chatterino
