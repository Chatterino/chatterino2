#pragma once

#include "channel.hpp"
#include "widgets/basewindow.hpp"

#include <pajlada/signals/signal.hpp>

class QLabel;

namespace chatterino {
namespace widgets {

class AccountPopup2 final : public BaseWindow
{
public:
    AccountPopup2();

    void setName(const QString &name);
    void setChannel(const ChannelPtr &_channel);

    struct {
        QLabel *label = nullptr;
    } ui;

protected:
    //    virtual void scaleChangedEvent(float newScale) override;

private:
    bool isMod_;
    bool isBroadcaster_;

    pajlada::Signals::NoArgSignal userStateChanged;

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
