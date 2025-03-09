#include "widgets/dialogs/SelectChannelDialog.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/Notebook.hpp"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

namespace chatterino {

constexpr int TAB_TWITCH = 0;

SelectChannelDialog::SelectChannelDialog(QWidget *parent)
    : BaseWindow(
          {
              BaseWindow::Flags::EnableCustomFrame,
              BaseWindow::Flags::Dialog,
              BaseWindow::DisableLayoutSave,
              BaseWindow::BoundsCheckOnShow,
          },
          parent)
    , selectedChannel_(Channel::getEmpty())
{
    this->setWindowTitle("Select a channel to join");

    this->tabFilter_.dialog = this;

    LayoutCreator<QWidget> layoutWidget(this->getLayoutContainer());
    auto layout = layoutWidget.setLayoutType<QVBoxLayout>().withoutMargin();
    auto notebook = layout.emplace<Notebook>(this).assign(&this->ui_.notebook);

    // twitch
    {
        LayoutCreator<QWidget> obj(new QWidget());
        auto vbox = obj.setLayoutType<QVBoxLayout>();

        // channel_btn
        auto channel_btn = vbox.emplace<QRadioButton>("Channel").assign(
            &this->ui_.twitch.channel);
        auto channel_lbl =
            vbox.emplace<QLabel>("Join a Twitch channel by its name.").hidden();
        channel_lbl->setWordWrap(true);
        auto channel_edit = vbox.emplace<QLineEdit>().hidden().assign(
            &this->ui_.twitch.channelName);

        QObject::connect(channel_btn.getElement(), &QRadioButton::toggled,
                         [=](bool enabled) mutable {
                             if (enabled)
                             {
                                 channel_edit->setFocus();
                                 channel_edit->setSelection(
                                     0, channel_edit->text().length());
                             }

                             channel_edit->setVisible(enabled);
                             channel_lbl->setVisible(enabled);
                         });

        channel_btn->installEventFilter(&this->tabFilter_);
        channel_edit->installEventFilter(&this->tabFilter_);

        // whispers_btn
        auto whispers_btn = vbox.emplace<QRadioButton>("Whispers")
                                .assign(&this->ui_.twitch.whispers);
        auto whispers_lbl =
            vbox.emplace<QLabel>("Shows the whispers that you receive while "
                                 "Chatterino is running.")
                .hidden();

        whispers_lbl->setWordWrap(true);
        whispers_btn->installEventFilter(&this->tabFilter_);

        QObject::connect(whispers_btn.getElement(), &QRadioButton::toggled,
                         [=](bool enabled) mutable {
                             whispers_lbl->setVisible(enabled);
                         });

        // mentions_btn
        auto mentions_btn = vbox.emplace<QRadioButton>("Mentions")
                                .assign(&this->ui_.twitch.mentions);
        auto mentions_lbl =
            vbox.emplace<QLabel>("Shows all the messages that highlight you "
                                 "from any channel.")
                .hidden();

        mentions_lbl->setWordWrap(true);
        mentions_btn->installEventFilter(&this->tabFilter_);

        QObject::connect(mentions_btn.getElement(), &QRadioButton::toggled,
                         [=](bool enabled) mutable {
                             mentions_lbl->setVisible(enabled);
                         });

        // watching_btn
        auto watching_btn = vbox.emplace<QRadioButton>("Watching")
                                .assign(&this->ui_.twitch.watching);
        auto watching_lbl =
            vbox.emplace<QLabel>("Requires the Chatterino browser extension.")
                .hidden();

        watching_lbl->setWordWrap(true);
        watching_btn->installEventFilter(&this->tabFilter_);

        QObject::connect(watching_btn.getElement(), &QRadioButton::toggled,
                         [=](bool enabled) mutable {
                             watching_lbl->setVisible(enabled);
                         });

        // live_btn
        auto live_btn =
            vbox.emplace<QRadioButton>("Live").assign(&this->ui_.twitch.live);
        auto live_lbl =
            vbox.emplace<QLabel>("Shows when channels go live.").hidden();

        live_lbl->setWordWrap(true);
        live_btn->installEventFilter(&this->tabFilter_);

        QObject::connect(live_btn.getElement(), &QRadioButton::toggled,
                         [=](bool enabled) mutable {
                             live_lbl->setVisible(enabled);
                         });

        // automod_btn
        auto automod_btn = vbox.emplace<QRadioButton>("AutoMod").assign(
            &this->ui_.twitch.automod);
        auto automod_lbl =
            vbox.emplace<QLabel>("Shows when AutoMod catches a message in any "
                                 "channel you moderate.")
                .hidden();

        automod_lbl->setWordWrap(true);
        automod_btn->installEventFilter(&this->tabFilter_);

        QObject::connect(automod_btn.getElement(), &QRadioButton::toggled,
                         [=](bool enabled) mutable {
                             automod_lbl->setVisible(enabled);
                         });

        vbox->addStretch(1);

        // tabbing order
        QWidget::setTabOrder(automod_btn.getElement(),
                             channel_btn.getElement());
        QWidget::setTabOrder(channel_btn.getElement(),
                             whispers_btn.getElement());
        QWidget::setTabOrder(whispers_btn.getElement(),
                             mentions_btn.getElement());
        QWidget::setTabOrder(mentions_btn.getElement(),
                             watching_btn.getElement());
        QWidget::setTabOrder(watching_btn.getElement(), live_btn.getElement());
        QWidget::setTabOrder(live_btn.getElement(), automod_btn.getElement());

        // tab
        auto *tab = notebook->addPage(obj.getElement());
        tab->setCustomTitle("Twitch");
    }

    layout->setStretchFactor(notebook.getElement(), 1);

    auto buttons =
        layout.emplace<QHBoxLayout>().emplace<QDialogButtonBox>(this);
    {
        auto *button_ok = buttons->addButton(QDialogButtonBox::Ok);
        QObject::connect(button_ok, &QPushButton::clicked, [this](bool) {
            this->ok();
        });
        auto *button_cancel = buttons->addButton(QDialogButtonBox::Cancel);
        QObject::connect(button_cancel, &QAbstractButton::clicked,
                         [this](bool) {
                             this->close();
                         });
    }

    this->setMinimumSize(300, 310);
    this->ui_.notebook->selectIndex(TAB_TWITCH);
    this->ui_.twitch.channel->setFocus();

    this->addShortcuts();

    this->themeChangedEvent();
}

void SelectChannelDialog::ok()
{
    // accept and close
    this->hasSelectedChannel_ = true;
    this->close();
}

void SelectChannelDialog::setSelectedChannel(IndirectChannel _channel)
{
    auto channel = _channel.get();

    assert(channel);

    this->selectedChannel_ = channel;

    switch (_channel.getType())
    {
        case Channel::Type::Twitch: {
            this->ui_.notebook->selectIndex(TAB_TWITCH);
            this->ui_.twitch.channel->setFocus();
            this->ui_.twitch.channelName->setText(channel->getName());
        }
        break;
        case Channel::Type::TwitchWatching: {
            this->ui_.notebook->selectIndex(TAB_TWITCH);
            this->ui_.twitch.watching->setFocus();
        }
        break;
        case Channel::Type::TwitchMentions: {
            this->ui_.notebook->selectIndex(TAB_TWITCH);
            this->ui_.twitch.mentions->setFocus();
        }
        break;
        case Channel::Type::TwitchWhispers: {
            this->ui_.notebook->selectIndex(TAB_TWITCH);
            this->ui_.twitch.whispers->setFocus();
        }
        break;
        case Channel::Type::TwitchLive: {
            this->ui_.notebook->selectIndex(TAB_TWITCH);
            this->ui_.twitch.live->setFocus();
        }
        break;
        case Channel::Type::TwitchAutomod: {
            this->ui_.notebook->selectIndex(TAB_TWITCH);
            this->ui_.twitch.automod->setFocus();
        }
        break;
        default: {
            this->ui_.notebook->selectIndex(TAB_TWITCH);
            this->ui_.twitch.channel->setFocus();
        }
    }

    this->hasSelectedChannel_ = false;
}

IndirectChannel SelectChannelDialog::getSelectedChannel() const
{
    if (!this->hasSelectedChannel_)
    {
        return this->selectedChannel_;
    }

    switch (this->ui_.notebook->getSelectedIndex())
    {
        case TAB_TWITCH: {
            if (this->ui_.twitch.channel->isChecked())
            {
                return getApp()->getTwitch()->getOrAddChannel(
                    this->ui_.twitch.channelName->text().trimmed());
            }
            else if (this->ui_.twitch.watching->isChecked())
            {
                return getApp()->getTwitch()->getWatchingChannel();
            }
            else if (this->ui_.twitch.mentions->isChecked())
            {
                return getApp()->getTwitch()->getMentionsChannel();
            }
            else if (this->ui_.twitch.whispers->isChecked())
            {
                return getApp()->getTwitch()->getWhispersChannel();
            }
            else if (this->ui_.twitch.live->isChecked())
            {
                return getApp()->getTwitch()->getLiveChannel();
            }
            else if (this->ui_.twitch.automod->isChecked())
            {
                return getApp()->getTwitch()->getAutomodChannel();
            }
        }
        break;
    }

    return this->selectedChannel_;
}

bool SelectChannelDialog::hasSeletedChannel() const
{
    return this->hasSelectedChannel_;
}

bool SelectChannelDialog::EventFilter::eventFilter(QObject *watched,
                                                   QEvent *event)
{
    auto *widget = static_cast<QWidget *>(watched);

    if (event->type() == QEvent::FocusIn)
    {
        auto *radio = dynamic_cast<QRadioButton *>(watched);
        if (radio)
        {
            radio->setChecked(true);
            return true;
        }
        else
        {
            return false;
        }
    }
    else if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *event_key = static_cast<QKeyEvent *>(event);
        if ((event_key->key() == Qt::Key_Tab ||
             event_key->key() == Qt::Key_Down) &&
            event_key->modifiers() == Qt::NoModifier)
        {
            // Tab has been pressed, focus next entry in list

            if (widget == this->dialog->ui_.twitch.channelName)
            {
                // Special case for when current selection is the "Channel" entry's edit box since the Edit box actually has the focus
                this->dialog->ui_.twitch.whispers->setFocus();
                return true;
            }
            else if (widget == this->dialog->ui_.twitch.automod)
            {
                // Special case for when current selection is "AutoMod" (the last entry in the list), next wrap is Channel, but we need to select its edit box
                this->dialog->ui_.twitch.channel->setFocus();
                return true;
            }

            widget->nextInFocusChain()->setFocus();
            return true;
        }
        else if (((event_key->key() == Qt::Key_Tab ||
                   event_key->key() == Qt::Key_Backtab) &&
                  event_key->modifiers() == Qt::ShiftModifier) ||
                 ((event_key->key() == Qt::Key_Up) &&
                  event_key->modifiers() == Qt::NoModifier))
        {
            // Shift+Tab has been pressed, focus previous entry in list

            if (widget == this->dialog->ui_.twitch.channelName)
            {
                // Special case for when current selection is the "Channel" entry's edit box since the Edit box actually has the focus
                this->dialog->ui_.twitch.automod->setFocus();
                return true;
            }

            widget->previousInFocusChain()->setFocus();
            return true;
        }
        else if (event_key == QKeySequence::DeleteStartOfWord &&
                 this->dialog->ui_.twitch.channelName->selectionLength() > 0)
        {
            this->dialog->ui_.twitch.channelName->backspace();
            return true;
        }
        else
        {
            return false;
        }
    }
    else if (event->type() == QEvent::KeyRelease)
    {
        QKeyEvent *event_key = static_cast<QKeyEvent *>(event);
        if ((event_key->key() == Qt::Key_Backtab ||
             event_key->key() == Qt::Key_Down) &&
            event_key->modifiers() == Qt::NoModifier)
        {
            return true;
        }
    }

    return false;
}

void SelectChannelDialog::closeEvent(QCloseEvent *)
{
    this->closed.invoke();
}

void SelectChannelDialog::themeChangedEvent()
{
    BaseWindow::themeChangedEvent();

    // NOTE: This currently overrides the QLineEdit's font size.
    // If the dialog is open when the theme is changed, things will look a bit off.
    // This can be alleviated by us using a single application-wide style sheet for these things.
    this->ui_.twitch.channelName->setStyleSheet(
        this->theme->splits.input.styleSheet);
}

void SelectChannelDialog::addShortcuts()
{
    HotkeyController::HotkeyMap actions{
        {"accept",
         [this](std::vector<QString>) -> QString {
             this->ok();
             return "";
         }},
        {"reject",
         [this](std::vector<QString>) -> QString {
             this->close();
             return "";
         }},

        // these make no sense, so they aren't implemented
        {"scrollPage", nullptr},
        {"search", nullptr},
        {"delete", nullptr},
        {"openTab", nullptr},
    };

    this->shortcuts_ = getApp()->getHotkeys()->shortcutsForCategory(
        HotkeyCategory::PopupWindow, actions, this);
}

}  // namespace chatterino
