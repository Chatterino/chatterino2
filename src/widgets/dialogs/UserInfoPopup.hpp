#pragma once

#include "widgets/DraggablePopup.hpp"

#include <pajlada/signals/scoped-connection.hpp>
#include <pajlada/signals/signal.hpp>
#include <QPointer>

#include <chrono>

class QCheckBox;

namespace chatterino {

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;
class Label;
class ChannelView;
class Split;

class UserInfoPopup final : public DraggablePopup
{
    Q_OBJECT

public:
    UserInfoPopup(bool closeAutomatically, QWidget *parent,
                  QPointer<Split> split = {});

    void setData(const QString &name, const ChannelPtr &channel);
    void setData(const QString &name, const ChannelPtr &contextChannel,
                 const ChannelPtr &openingChannel);

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

    QPointer<Split> split_;

    QString userName_;
    QString userId_;
    QString avatarUrl_;

    // The channel the popup was opened from (e.g. /mentions or #forsen). Can be a special channel.
    ChannelPtr channel_;

    // The channel the messages are rendered from (e.g. #forsen). Can be a special channel, but will try to not be where possible.
    ChannelPtr underlyingChannel_;

    pajlada::Signals::NoArgSignal userStateChanged_;

    std::unique_ptr<pajlada::Signals::ScopedConnection> refreshConnection_;

    // If we should close the dialog automatically if the user clicks out
    // Set based on the "Automatically close usercard when it loses focus" setting
    // Pinned status is tracked in DraggablePopup::isPinned_.
    const bool closeAutomatically_;

    struct {
        Button *avatarButton = nullptr;
        Button *localizedNameCopyButton = nullptr;

        Label *nameLabel = nullptr;
        Label *localizedNameLabel = nullptr;
        Label *followerCountLabel = nullptr;
        Label *createdDateLabel = nullptr;
        Label *userIDLabel = nullptr;
        Label *followageLabel = nullptr;
        Label *subageLabel = nullptr;

        QCheckBox *block = nullptr;
        QCheckBox *ignoreHighlights = nullptr;

        Label *noMessagesLabel = nullptr;
        ChannelView *latestMessages = nullptr;
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
