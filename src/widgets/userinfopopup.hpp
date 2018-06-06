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
public:
    UserInfoPopup();

    void setData(const QString &name, const ChannelPtr &channel);

private:
    bool isMod_;
    bool isBroadcaster_;

    QString userName_;
    ChannelPtr channel_;

    pajlada::Signals::NoArgSignal userStateChanged;

    void updateUserData();
    void loadAvatar(const QUrl &url);

    struct {
        RippleEffectButton *avatarButton = nullptr;

        QLabel *nameLabel = nullptr;
        QLabel *viewCountLabel = nullptr;
        QLabel *followerCountLabel = nullptr;
        QLabel *createdDateLabel = nullptr;

        QCheckBox *ignore = nullptr;
        QCheckBox *ignoreHighlights = nullptr;
    } ui_;

    class TimeoutWidget : public BaseWidget
    {
    public:
        TimeoutWidget();

    protected:
        void paintEvent(QPaintEvent *event) override;
    };
};

}  // namespace widgets
}  // namespace chatterino
