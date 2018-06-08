#pragma once

#include "channel.hpp"
#include "widgets/basewindow.hpp"

#include <pajlada/signals/signal.hpp>

class QLabel;
class QCheckBox;

namespace chatterino {
namespace widgets {

class UserInfoPopup final : public BaseWindow
{
    Q_OBJECT

public:
    UserInfoPopup();

    void setData(const QString &name, const ChannelPtr &channel);

private:
    bool isMod_;
    bool isBroadcaster_;

    QString userName_;
    QString userId_;
    ChannelPtr channel_;

    pajlada::Signals::NoArgSignal userStateChanged;

    void installEvents();

    void updateUserData();
    void loadAvatar(const QUrl &url);

    std::shared_ptr<bool> hack_;

    struct {
        RippleEffectButton *avatarButton = nullptr;

        QLabel *nameLabel = nullptr;
        QLabel *viewCountLabel = nullptr;
        QLabel *followerCountLabel = nullptr;
        QLabel *createdDateLabel = nullptr;

        QCheckBox *follow = nullptr;
        QCheckBox *ignore = nullptr;
        QCheckBox *ignoreHighlights = nullptr;
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

}  // namespace widgets
}  // namespace chatterino
