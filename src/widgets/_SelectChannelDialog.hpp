#pragma once

#include "channel.hpp"
#include "widgets/basewindow.hpp"
#include "widgets/notebook.hpp"

#include <pajlada/signals/signal.hpp>

#include <QLabel>
#include <QRadioButton>

namespace chatterino {
namespace widgets {

class SelectChannelDialog : public BaseWindow
{
public:
    SelectChannelDialog(QWidget *parent = nullptr);

    void setSelectedChannel(IndirectChannel selectedChannel);
    IndirectChannel getSelectedChannel() const;
    bool hasSeletedChannel() const;

    pajlada::Signals::NoArgSignal closed;

protected:
    virtual void closeEvent(QCloseEvent *) override;
    virtual void themeRefreshEvent() override;

private:
    class EventFilter : public QObject
    {
    public:
        SelectChannelDialog *dialog;

    protected:
        virtual bool eventFilter(QObject *watched, QEvent *event) override;
    };

    struct {
        Notebook *notebook;
        struct {
            QRadioButton *channel;
            QLineEdit *channelName;
            QRadioButton *whispers;
            QRadioButton *mentions;
            QRadioButton *watching;
        } twitch;
    } ui_;

    EventFilter tabFilter;

    ChannelPtr selectedChannel;
    bool _hasSelectedChannel = false;

    void ok();
    friend class EventFilter;
};

}  // namespace widgets
}  // namespace chatterino
