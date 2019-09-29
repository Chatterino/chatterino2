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

    void setSelectedChannel(IndirectChannel _channel);
    std::vector<IndirectChannel> getSelectedChannels() const;
    bool hasSelectedChannels() const;

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
            QCheckBox *channels;
            QLineEdit *channelNames;
            QCheckBox *whispers;
            QCheckBox *mentions;
            QCheckBox *watching;
        } twitch;
        struct {
            QLineEdit *channel;
            EditableModelView *servers;
        } irc;
    } ui_;

    EventFilter keyboardFilter_;

    std::vector<IndirectChannel> selectedChannels_;
    bool hasSelectedChannels_ = false;

    void ok();
    friend class EventFilter;
};

}  // namespace chatterino
