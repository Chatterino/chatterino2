#pragma once

#include "widgets/BaseWindow.hpp"
#include "widgets/helper/ChannelView.hpp"

#include <pajlada/signals/scoped-connection.hpp>
#include <pajlada/signals/signal.hpp>

#include <chrono>

class QCheckBox;

namespace chatterino {

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;
class Label;

class UserInfoPopup final : public BaseWindow
{
    Q_OBJECT

public:
    UserInfoPopup(bool closeAutomatically, QWidget *parent);

    void setData(const QString &name, const ChannelPtr &channel);

protected:
    virtual void themeChangedEvent() override;
    virtual void scaleChangedEvent(float scale) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void installEvents();
    void updateUserData();
    void updateLatestMessages();

    void loadAvatar(const QUrl &url);
    bool isMod_;
    bool isBroadcaster_;

    QString userName_;
    QString userId_;
    QString avatarUrl_;
    ChannelPtr channel_;

    // isMoving_ is set to true if the user is holding the left mouse button down and has moved the mouse a small amount away from the original click point (startPosDrag_)
    bool isMoving_ = false;

    // startPosDrag_ is the coordinates where the user originally pressed the mouse button down to start dragging
    QPoint startPosDrag_;

    // requestDragPos_ is the final screen coordinates where the widget should be moved to.
    // Takes the relative position of where the user originally clicked the widget into account
    QPoint requestedDragPos_;

    // dragTimer_ is called ~60 times per second once the user has initiated dragging
    QTimer dragTimer_;

    pajlada::Signals::NoArgSignal userStateChanged_;

    std::unique_ptr<pajlada::Signals::ScopedConnection> refreshConnection_;

    std::shared_ptr<bool> hack_;
    std::mutex checkAfkRateLimiter_;

    struct {
        Button *avatarButton = nullptr;
        Button *localizedNameCopyButton = nullptr;

        Label *nameLabel = nullptr;
        Label *localizedNameLabel = nullptr;
        Label *viewCountLabel = nullptr;
        Label *followerCountLabel = nullptr;
        Label *createdDateLabel = nullptr;
        Label *userIDLabel = nullptr;
        Label *followageLabel = nullptr;
        Label *subageLabel = nullptr;

        QCheckBox *block = nullptr;
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
