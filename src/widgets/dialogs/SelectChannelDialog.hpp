#pragma once

#include "widgets/BaseWindow.hpp"

#include <pajlada/signals/signal.hpp>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>

namespace chatterino {

class Notebook;
class EditableModelView;
class IndirectChannel;
class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

class SelectChannelDialog final : public BaseWindow
{
public:
    SelectChannelDialog(QWidget *parent = nullptr);

    void setSelectedChannel(IndirectChannel selectedChannel_);
    IndirectChannel getSelectedChannel() const;
    bool hasSeletedChannel() const;

    pajlada::Signals::NoArgSignal closed;

protected:
    void closeEvent(QCloseEvent *) override;
    void themeChangedEvent() override;

private:
    class EventFilter : public QObject
    {
    public:
        SelectChannelDialog *dialog;

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;
    };

    struct {
        Notebook *notebook;
        struct {
            QRadioButton *channel;
            QLineEdit *channelName;
            QRadioButton *whispers;
            QRadioButton *mentions;
            QRadioButton *watching;
            QRadioButton *live;
            QRadioButton *automod;
        } twitch;
    } ui_;

    EventFilter tabFilter_;

    ChannelPtr selectedChannel_;
    bool hasSelectedChannel_ = false;

    void ok();
    friend class EventFilter;

    void addShortcuts() override;
};

}  // namespace chatterino
