// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/dialogs/SelectChannelDialog.hpp"

#include "Application.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Theme.hpp"

#include <QDialogButtonBox>
#include <QEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

namespace chatterino {

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
    using AutoCheckedRadioButton = detail::AutoCheckedRadioButton;

    this->setWindowTitle("Select a channel to join");

    this->tabFilter_.dialog = this;

    auto *layout = new QVBoxLayout(this->getLayoutContainer());

    auto &ui = this->ui_;
    // Channel
    ui.channel = new AutoCheckedRadioButton("Channel");
    layout->addWidget(ui.channel);

    ui.channelLabel = new QLabel("Join a Twitch channel by its channel name");
    ui.channelLabel->setVisible(false);
    layout->addWidget(ui.channelLabel);

    ui.channelName = new QLineEdit();
    ui.channelName->setVisible(false);
    layout->addWidget(ui.channelName);

    QObject::connect(ui.channel, &AutoCheckedRadioButton::toggled, this,
                     [this](bool enabled) {
                         auto &ui = this->ui_;
                         ui.channelName->setVisible(enabled);
                         ui.channelLabel->setVisible(enabled);

                         if (enabled)
                         {
                             ui.channelName->setFocus();
                             ui.channelName->selectAll();
                         }
                     });

    ui.channel->installEventFilter(&this->tabFilter_);
    ui.channelName->installEventFilter(&this->tabFilter_);

    // Whispers
    ui.whispers = new AutoCheckedRadioButton("Whispers");
    layout->addWidget(ui.whispers);

    ui.whispersLabel = new QLabel(
        "Shows the whispers that you receive while Chatterino is running");
    ui.whispersLabel->setVisible(false);
    ui.whispersLabel->setWordWrap(true);
    layout->addWidget(ui.whispersLabel);

    QObject::connect(ui.whispers, &AutoCheckedRadioButton::toggled, this,
                     [this](bool enabled) {
                         auto &ui = this->ui_;
                         ui.whispersLabel->setVisible(enabled);
                     });

    ui.whispers->installEventFilter(&this->tabFilter_);

    // Mentions
    ui.mentions = new AutoCheckedRadioButton("Mentions");
    layout->addWidget(ui.mentions);

    ui.mentionsLabel = new QLabel(
        "Shows all the messages that highlight you from any channel");
    ui.mentionsLabel->setVisible(false);
    ui.mentionsLabel->setWordWrap(true);
    layout->addWidget(ui.mentionsLabel);

    QObject::connect(ui.mentions, &AutoCheckedRadioButton::toggled, this,
                     [this](bool enabled) {
                         auto &ui = this->ui_;
                         ui.mentionsLabel->setVisible(enabled);
                     });

    ui.mentions->installEventFilter(&this->tabFilter_);

    // Watching
    ui.watching = new AutoCheckedRadioButton("Watching");
    layout->addWidget(ui.watching);

    ui.watchingLabel = new QLabel("Requires the Chatterino browser extension");
    ui.watchingLabel->setVisible(false);
    layout->addWidget(ui.watchingLabel);

    QObject::connect(ui.watching, &AutoCheckedRadioButton::toggled, this,
                     [this](bool enabled) {
                         auto &ui = this->ui_;
                         ui.watchingLabel->setVisible(enabled);
                     });

    ui.watching->installEventFilter(&this->tabFilter_);

    // Live
    ui.live = new AutoCheckedRadioButton("Live");
    layout->addWidget(ui.live);

    ui.liveLabel = new QLabel("Shows when channels go live");
    ui.liveLabel->setVisible(false);
    layout->addWidget(ui.liveLabel);

    QObject::connect(ui.live, &AutoCheckedRadioButton::toggled, this,
                     [this](bool enabled) {
                         auto &ui = this->ui_;
                         ui.liveLabel->setVisible(enabled);
                     });

    ui.live->installEventFilter(&this->tabFilter_);

    // Automod
    ui.automod = new AutoCheckedRadioButton("AutoMod");
    layout->addWidget(ui.automod);

    ui.automodLabel = new QLabel("Shows when AutoMod catches a message in "
                                 "any channel you moderate.");
    ui.automodLabel->setVisible(false);
    ui.automodLabel->setWordWrap(true);
    layout->addWidget(ui.automodLabel);

    QObject::connect(ui.automod, &AutoCheckedRadioButton::toggled, this,
                     [this](bool enabled) {
                         auto &ui = this->ui_;
                         ui.automodLabel->setVisible(enabled);
                     });

    ui.automod->installEventFilter(&this->tabFilter_);

    layout->addStretch(1);

    auto *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, [this] {
        this->ok();
    });
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, [this] {
        this->close();
    });

    this->addShortcuts();

    this->themeChangedEvent();
}

void SelectChannelDialog::ok()
{
    // accept and close
    this->hasSelectedChannel_ = true;
    this->close();
}

void SelectChannelDialog::setSelectedChannel(
    std::optional<IndirectChannel> channel_)
{
    if (!channel_.has_value())
    {
        this->ui_.channel->setChecked(true);

        this->hasSelectedChannel_ = false;
        return;
    }

    const auto &indirectChannel = channel_.value();
    const auto &channel = indirectChannel.get();

    assert(channel);

    this->selectedChannel_ = channel;

    switch (indirectChannel.getType())
    {
        case Channel::Type::Twitch: {
            this->ui_.channelName->setText(channel->getName());
            this->ui_.channel->setChecked(true);
        }
        break;
        case Channel::Type::TwitchWatching: {
            this->ui_.watching->setFocus();
        }
        break;
        case Channel::Type::TwitchMentions: {
            this->ui_.mentions->setFocus();
        }
        break;
        case Channel::Type::TwitchWhispers: {
            this->ui_.whispers->setFocus();
        }
        break;
        case Channel::Type::TwitchLive: {
            this->ui_.live->setFocus();
        }
        break;
        case Channel::Type::TwitchAutomod: {
            this->ui_.automod->setFocus();
        }
        break;
        default: {
            this->ui_.channel->setChecked(true);
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

    if (this->ui_.channel->isChecked())
    {
        return getApp()->getTwitch()->getOrAddChannel(
            this->ui_.channelName->text().trimmed());
    }

    if (this->ui_.watching->isChecked())
    {
        return getApp()->getTwitch()->getWatchingChannel();
    }

    if (this->ui_.mentions->isChecked())
    {
        return getApp()->getTwitch()->getMentionsChannel();
    }

    if (this->ui_.whispers->isChecked())
    {
        return getApp()->getTwitch()->getWhispersChannel();
    }

    if (this->ui_.live->isChecked())
    {
        return getApp()->getTwitch()->getLiveChannel();
    }

    if (this->ui_.automod->isChecked())
    {
        return getApp()->getTwitch()->getAutomodChannel();
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
    auto *widget = dynamic_cast<QWidget *>(watched);
    assert(widget);

    auto &ui = this->dialog->ui_;

    if (event->type() == QEvent::KeyPress)
    {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
        assert(keyEvent);

        if ((keyEvent->key() == Qt::Key_Tab ||
             keyEvent->key() == Qt::Key_Down) &&
            keyEvent->modifiers() == Qt::NoModifier)
        {
            // Tab has been pressed, focus next entry in list

            if (widget == ui.channelName)
            {
                // Special case for when current selection is the "Channel" entry's edit box since the Edit box actually has the focus
                ui.whispers->setFocus();
                return true;
            }

            if (widget == ui.automod)
            {
                // Special case for when current selection is "AutoMod" (the last entry in the list), next wrap is Channel, but we need to select its edit box
                ui.channel->setFocus();
                return true;
            }

            auto *nextInFocusChain = widget->nextInFocusChain();
            if (nextInFocusChain->focusPolicy() == Qt::FocusPolicy::NoFocus)
            {
                // Make sure we're not selecting one of the labels
                nextInFocusChain = nextInFocusChain->nextInFocusChain();
            }
            nextInFocusChain->setFocus();
            return true;
        }

        if (((keyEvent->key() == Qt::Key_Tab ||
              keyEvent->key() == Qt::Key_Backtab) &&
             keyEvent->modifiers() == Qt::ShiftModifier) ||
            ((keyEvent->key() == Qt::Key_Up) &&
             keyEvent->modifiers() == Qt::NoModifier))
        {
            // Shift+Tab has been pressed, focus previous entry in list

            if (widget == ui.channelName)
            {
                // Special case for when current selection is the "Channel" entry's edit box since the Edit box actually has the focus
                ui.automod->setFocus();
                return true;
            }

            if (widget == ui.whispers)
            {
                ui.channel->setFocus();
                return true;
            }

            auto *previousInFocusChain = widget->previousInFocusChain();
            if (previousInFocusChain->focusPolicy() == Qt::FocusPolicy::NoFocus)
            {
                // Make sure we're not selecting one of the labels
                previousInFocusChain =
                    previousInFocusChain->previousInFocusChain();
            }
            previousInFocusChain->setFocus();
            return true;
        }

        if (keyEvent == QKeySequence::DeleteStartOfWord &&
            ui.channelName->selectionLength() > 0)
        {
            ui.channelName->backspace();
            return true;
        }

        return false;
    }

    return false;
}

void SelectChannelDialog::closeEvent(QCloseEvent * /*event*/)
{
    this->closed.invoke();
}

void SelectChannelDialog::themeChangedEvent()
{
    BaseWindow::themeChangedEvent();

    this->setPalette(getTheme()->palette);
}

void SelectChannelDialog::scaleChangedEvent(float newScale)
{
    BaseWindow::scaleChangedEvent(newScale);

    auto &ui = this->ui_;

    // NOTE: Normally the font is automatically inherited from its parent, but since we override
    // the style sheet to respect light/dark theme, we have to manually update the font here
    auto uiFont =
        getApp()->getFonts()->getFont(FontStyle::UiMedium, this->scale());

    ui.channelName->setFont(uiFont);
}

void SelectChannelDialog::addShortcuts()
{
    HotkeyController::HotkeyMap actions{
        {"accept",
         [this](const std::vector<QString> &) -> QString {
             this->ok();
             return "";
         }},
        {"reject",
         [this](const std::vector<QString> &) -> QString {
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
