#include "selectchanneldialog.hpp"

#include "application.hpp"
#include "providers/twitch/twitchserver.hpp"
#include "util/layoutcreator.hpp"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <widgets/notebook.hpp>

#define TAB_TWITCH 0

namespace chatterino {
namespace widgets {

SelectChannelDialog::SelectChannelDialog()
    : BaseWindow((QWidget *)nullptr, BaseWindow::EnableCustomFrame)
    , selectedChannel(Channel::getEmpty())
{
    this->setWindowTitle("Select a channel to join");

    this->tabFilter.dialog = this;

    util::LayoutCreator<QWidget> layoutWidget(this->getLayoutContainer());
    auto layout = layoutWidget.setLayoutType<QVBoxLayout>().withoutMargin();
    auto notebook = layout.emplace<Notebook>(this).assign(&this->ui_.notebook);

    // twitch
    {
        util::LayoutCreator<QWidget> obj(new QWidget());
        auto vbox = obj.setLayoutType<QVBoxLayout>();

        // channel_btn
        auto channel_btn = vbox.emplace<QRadioButton>("Channel").assign(&this->ui_.twitch.channel);
        auto channel_lbl = vbox.emplace<QLabel>("Join a twitch channel by its name.").hidden();
        channel_lbl->setWordWrap(true);
        auto channel_edit = vbox.emplace<QLineEdit>().hidden().assign(&this->ui_.twitch.channelName);

        QObject::connect(*channel_btn, &QRadioButton::toggled, [=](bool enabled) mutable {
            if (enabled) {
                channel_edit->setFocus();
                channel_edit->setSelection(0, channel_edit->text().length());
            }

            channel_edit->setVisible(enabled);
            channel_lbl->setVisible(enabled);
        });

        channel_btn->installEventFilter(&this->tabFilter);
        channel_edit->installEventFilter(&this->tabFilter);

        // whispers_btn
        auto whispers_btn =
            vbox.emplace<QRadioButton>("Whispers").assign(&this->ui_.twitch.whispers);
        auto whispers_lbl =
            vbox.emplace<QLabel>("Shows the whispers that you receive while chatterino is running.")
                .hidden();

        whispers_lbl->setWordWrap(true);
        whispers_btn->installEventFilter(&this->tabFilter);

        QObject::connect(*whispers_btn, &QRadioButton::toggled,
                         [=](bool enabled) mutable { whispers_lbl->setVisible(enabled); });

        // mentions_btn
        auto mentions_btn =
            vbox.emplace<QRadioButton>("Mentions").assign(&this->ui_.twitch.mentions);
        auto mentions_lbl =
            vbox.emplace<QLabel>("Shows all the messages that highlight you from any channel.")
                .hidden();

        mentions_lbl->setWordWrap(true);
        mentions_btn->installEventFilter(&this->tabFilter);

        QObject::connect(*mentions_btn, &QRadioButton::toggled,
                         [=](bool enabled) mutable { mentions_lbl->setVisible(enabled); });

        // watching_btn
        auto watching_btn =
            vbox.emplace<QRadioButton>("Watching").assign(&this->ui_.twitch.watching);
        auto watching_lbl =
            vbox.emplace<QLabel>("Requires the chatterino browser extension.").hidden();

        watching_lbl->setWordWrap(true);
        watching_btn->installEventFilter(&this->tabFilter);

        QObject::connect(*watching_btn, &QRadioButton::toggled,
                         [=](bool enabled) mutable { watching_lbl->setVisible(enabled); });

        vbox->addStretch(1);

        // tabbing order
        QWidget::setTabOrder(*watching_btn, *channel_btn);
        QWidget::setTabOrder(*channel_btn, *whispers_btn);
        QWidget::setTabOrder(*whispers_btn, *mentions_btn);
        QWidget::setTabOrder(*mentions_btn, *watching_btn);

        // tab
        NotebookTab *tab = notebook->addPage(obj.getElement());
        tab->setCustomTitle("Twitch");
    }

    // irc
    /*{
        util::LayoutCreator<QWidget> obj(new QWidget());
        auto vbox = obj.setLayoutType<QVBoxLayout>();

        auto edit = vbox.emplace<QLabel>("not implemented");

        NotebookTab2 *tab = notebook->addPage(obj.getElement());
        tab->setTitle("Irc");
    }*/

    layout->setStretchFactor(*notebook, 1);

    auto buttons = layout.emplace<QHBoxLayout>().emplace<QDialogButtonBox>(this);
    {
        auto *button_ok = buttons->addButton(QDialogButtonBox::Ok);
        QObject::connect(button_ok, &QPushButton::clicked, [=](bool) { this->ok(); });
        auto *button_cancel = buttons->addButton(QDialogButtonBox::Cancel);
        QObject::connect(button_cancel, &QAbstractButton::clicked, [=](bool) { this->close(); });
    }

    this->setScaleIndependantSize(300, 210);

    // Shortcuts
    auto *shortcut_ok = new QShortcut(QKeySequence("Return"), this);
    QObject::connect(shortcut_ok, &QShortcut::activated, [=] { this->ok(); });
    auto *shortcut_cancel = new QShortcut(QKeySequence("Esc"), this);
    QObject::connect(shortcut_cancel, &QShortcut::activated, [=] { this->close(); });
}

void SelectChannelDialog::ok()
{
    this->_hasSelectedChannel = true;
    this->close();
}

void SelectChannelDialog::setSelectedChannel(IndirectChannel _channel)
{
    auto channel = _channel.get();

    assert(channel);

    this->selectedChannel = channel;

    switch (_channel.getType()) {
        case Channel::Twitch: {
            this->ui_.notebook->selectIndex(TAB_TWITCH);
            this->ui_.twitch.channel->setFocus();
            this->ui_.twitch.channelName->setText(channel->name);
        } break;
        case Channel::TwitchWatching: {
            this->ui_.notebook->selectIndex(TAB_TWITCH);
            this->ui_.twitch.watching->setFocus();
        } break;
        case Channel::TwitchMentions: {
            this->ui_.notebook->selectIndex(TAB_TWITCH);
            this->ui_.twitch.mentions->setFocus();
        } break;
        case Channel::TwitchWhispers: {
            this->ui_.notebook->selectIndex(TAB_TWITCH);
            this->ui_.twitch.whispers->setFocus();
        } break;
        default: {
            this->ui_.notebook->selectIndex(TAB_TWITCH);
            this->ui_.twitch.channel->setFocus();
        }
    }

    this->_hasSelectedChannel = false;
}

IndirectChannel SelectChannelDialog::getSelectedChannel() const
{
    if (!this->_hasSelectedChannel) {
        return this->selectedChannel;
    }

    auto app = getApp();

    switch (this->ui_.notebook->getSelectedIndex()) {
        case TAB_TWITCH: {
            if (this->ui_.twitch.channel->isChecked()) {
                return app->twitch.server->getOrAddChannel(this->ui_.twitch.channelName->text());
            } else if (this->ui_.twitch.watching->isChecked()) {
                return app->twitch.server->watchingChannel;
            } else if (this->ui_.twitch.mentions->isChecked()) {
                return app->twitch.server->mentionsChannel;
            } else if (this->ui_.twitch.whispers->isChecked()) {
                return app->twitch.server->whispersChannel;
            }
        }
    }

    return this->selectedChannel;
}

bool SelectChannelDialog::hasSeletedChannel() const
{
    return this->_hasSelectedChannel;
}

bool SelectChannelDialog::EventFilter::eventFilter(QObject *watched, QEvent *event)
{
    auto *widget = (QWidget *)watched;

    if (event->type() == QEvent::FocusIn) {
        widget->grabKeyboard();

        auto *radio = dynamic_cast<QRadioButton *>(watched);
        if (radio) {
            radio->setChecked(true);
        }

        return true;
    } else if (event->type() == QEvent::FocusOut) {
        widget->releaseKeyboard();
        return false;
    } else if (event->type() == QEvent::KeyPress) {
        QKeyEvent *event_key = static_cast<QKeyEvent *>(event);
        if ((event_key->key() == Qt::Key_Tab || event_key->key() == Qt::Key_Down) &&
            event_key->modifiers() == Qt::NoModifier) {
            if (widget == this->dialog->ui_.twitch.channelName) {
                this->dialog->ui_.twitch.whispers->setFocus();
                return true;
            } else {
                widget->nextInFocusChain()->setFocus();
            }
            return true;
        } else if (((event_key->key() == Qt::Key_Tab || event_key->key() == Qt::Key_Backtab) &&
                    event_key->modifiers() == Qt::ShiftModifier) ||
                   ((event_key->key() == Qt::Key_Up) && event_key->modifiers() == Qt::NoModifier)) {
            if (widget == this->dialog->ui_.twitch.channelName) {
                this->dialog->ui_.twitch.watching->setFocus();
                return true;
            } else if (widget == this->dialog->ui_.twitch.whispers) {
                this->dialog->ui_.twitch.channel->setFocus();
                return true;
            }

            widget->previousInFocusChain()->setFocus();
            return true;
        } else {
            return false;
        }
        return true;
    } else if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *event_key = static_cast<QKeyEvent *>(event);
        if ((event_key->key() == Qt::Key_Backtab || event_key->key() == Qt::Key_Down) &&
            event_key->modifiers() == Qt::NoModifier) {
            return true;
        }
    }

    return false;
}

void SelectChannelDialog::closeEvent(QCloseEvent *)
{
    this->closed.invoke();
}

void SelectChannelDialog::themeRefreshEvent()
{
    BaseWindow::themeRefreshEvent();

    if (this->themeManager->isLightTheme()) {
        this->setStyleSheet("QRadioButton { color: #000 } QLabel { color: #000 }");
    } else {
        this->setStyleSheet("QRadioButton { color: #fff } QLabel { color: #fff }");
    }
}

}  // namespace widgets
}  // namespace chatterino
