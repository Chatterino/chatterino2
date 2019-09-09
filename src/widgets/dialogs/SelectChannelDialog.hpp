#pragma once

#include "Application.hpp"
#include "common/Channel.hpp"
#include "widgets/BaseWindow.hpp"

#include <pajlada/signals/signal.hpp>

#include <QLabel>
#include <QRadioButton>

namespace chatterino {

class Notebook;
class EditableModelView;

class SelectChannelDialog final : public BaseWindow
{
public:
    SelectChannelDialog(QWidget *parent = nullptr);

    void setSelectedChannel(IndirectChannel selectedChannel_);
    IndirectChannel getSelectedChannel() const;
    bool hasSeletedChannel() const;

    pajlada::Signals::NoArgSignal closed;

protected:
    virtual void closeEvent(QCloseEvent *) override;
    virtual void themeChangedEvent() override;

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
        struct {
            QLineEdit *channel;
            EditableModelView *servers;
        } irc;
    } ui_;

    EventFilter tabFilter_;

    ChannelPtr selectedChannel_;
    bool hasSelectedChannel_ = false;

    void ok();
    friend class EventFilter;
};

}  // namespace chatterino
