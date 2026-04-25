// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/dialogs/SelectChannelDialog.hpp"

#include "Application.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "controllers/plugins/api/ChannelProviders.hpp"
#include "controllers/plugins/PluginChannel.hpp"
#include "controllers/plugins/PluginController.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Theme.hpp"
#include "util/Variant.hpp"
#include "util/WeakPtrHelpers.hpp"
#include "widgets/helper/MicroNotebook.hpp"

#include <QComboBox>
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

namespace {

using namespace chatterino;

struct WeakChannelProvider {
#ifdef CHATTERINO_HAVE_PLUGINS
    std::weak_ptr<lua::api::ChannelProvider> value;

    std::shared_ptr<lua::api::ChannelProvider> lock() const
    {
        return this->value.lock();
    }

    bool operator==(const WeakChannelProvider &other) const noexcept
    {
        return weakOwnerEquals(this->value, other.value);
    }
#endif
};

}  // namespace

Q_DECLARE_METATYPE(WeakChannelProvider)

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

    auto &ui = this->ui_;
    auto *rootLayout = new QVBoxLayout(this->getLayoutContainer());
    rootLayout->setContentsMargins({});
    ui.notebook = new MicroNotebook(this->getLayoutContainer());
    rootLayout->addWidget(ui.notebook, 1);

    ui.twitchPage = new QWidget;
    auto *layout = new QVBoxLayout(ui.twitchPage);

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

    ui.notebook->addPage(ui.twitchPage, "Twitch");

    // Plugin page
    std::vector<std::pair<QString, WeakChannelProvider>> providers;
#ifdef CHATTERINO_HAVE_PLUGINS
    const auto &plugins = getApp()->getPlugins()->plugins();
    for (const auto &[pluginID, plugin] : plugins)
    {
        for (const auto &[providerID, provider] : plugin->channelProviders())
        {
            auto name =
                provider->displayName() % u" (" % plugin->meta.name % ')';
            providers.emplace_back(name, WeakChannelProvider(provider));
        }
    }
#endif

    if (providers.empty())
    {
        this->ui_.notebook->setShowHeader(false);
    }
    else
    {
#ifdef CHATTERINO_HAVE_PLUGINS
        ui.pluginPage = new QWidget;
        auto *layout = new QVBoxLayout(ui.pluginPage);

        ui.pluginProviderBox = new QComboBox;
        for (const auto &[name, data] : providers)
        {
            ui.pluginProviderBox->addItem(name, QVariant::fromValue(data));
        }
        QObject::connect(ui.pluginProviderBox, &QComboBox::currentIndexChanged,
                         this, [this] {
                             this->refreshChannelProviderWidgets();
                         });
        layout->addWidget(ui.pluginProviderBox);

        auto *content = new QWidget;
        ui.pluginProviderArgumentsLayout = new QFormLayout(content);
        layout->addWidget(content);

        layout->addStretch(1);

        ui.notebook->addPage(ui.pluginPage, "Plugins");

        this->refreshChannelProviderWidgets();
#endif
    }

    auto *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->setContentsMargins({10, 10, 10, 10});
    rootLayout->addWidget(buttonBox);

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
#ifdef CHATTERINO_HAVE_PLUGINS
        case Channel::Type::Plugin: {
            this->ui_.notebook->select(this->ui_.pluginPage);
            if (auto *pc = dynamic_cast<PluginChannel *>(channel.get()))
            {
                auto provider = getApp()->getPlugins()->findProvider(
                    pc->pluginID(), pc->providerID());
                if (provider)
                {
                    auto idx = this->ui_.pluginProviderBox->findData(
                        QVariant::fromValue(WeakChannelProvider(provider)));
                    if (idx >= 0)
                    {
                        this->ui_.pluginProviderBox->setCurrentIndex(idx);
                        this->applyChannelProviderArguments(pc->arguments());
                    }
                }
            }
        }
        break;
#endif
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

#ifdef CHATTERINO_HAVE_PLUGINS
    if (this->ui_.notebook->isSelected(this->ui_.pluginPage))
    {
        auto provider = this->ui_.pluginProviderBox->currentData()
                            .value<WeakChannelProvider>()
                            .lock();
        if (!provider)
        {
            return Channel::getEmpty();
        }
        return getApp()->getPlugins()->getOrCreatePluginChannelFromDialog(
            provider, this->extractChannelProviderArguments());
    }
#endif

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

#ifdef CHATTERINO_HAVE_PLUGINS
void SelectChannelDialog::refreshChannelProviderWidgets()
{
    auto provider = this->ui_.pluginProviderBox->currentData()
                        .value<WeakChannelProvider>()
                        .lock();
    // Clear previous callbacks.
    this->pluginControlCallbacks.clear();

    // Clear previous widgets.
    auto *layout = this->ui_.pluginProviderArgumentsLayout;
    while (layout->count() > 0)
    {
        auto *item = layout->takeAt(0);
        assert(item && item->layout() == nullptr);
        delete item->widget();
        delete item;
    }

    if (!provider)
    {
        return;
    }

    // Add the new widgets.
    if (!provider->description().isEmpty())
    {
        layout->addRow(new QLabel(provider->description()));
    }

    for (const auto &arg : provider->arguments())
    {
        QWidget *actionWidget = createChannelProviderArgumentWidget(arg);

        auto *label = new QLabel(arg.displayName);
        label->setToolTip(arg.tooltip);
        layout->addRow(label, actionWidget);
    }
}

QJsonObject SelectChannelDialog::extractChannelProviderArguments() const
{
    QJsonObject obj;
    for (const auto &fns : this->pluginControlCallbacks)
    {
        fns.applyToArguments(obj);
    }
    return obj;
}

void SelectChannelDialog::applyChannelProviderArguments(
    const QJsonObject &arguments)
{
    for (const auto &fns : this->pluginControlCallbacks)
    {
        fns.applyFromArguments(arguments);
    }
}

QWidget *SelectChannelDialog::createChannelProviderArgumentWidget(
    const lua::api::channelproviders::ArgumentSpec &arg)
{
    using namespace lua::api::channelproviders;

    return std::visit(
        variant::Overloaded{
            [&](const TextSpec &text) -> QWidget * {
                auto *edit = new QLineEdit(text.defaultValue);
                edit->setPlaceholderText(text.placeholder);

                auto fromArgs = [ptr = QPointer(edit),
                                 id = arg.id](const QJsonObject &obj) {
                    if (!ptr)
                    {
                        return;
                    }
                    auto val = obj.value(id);
                    if (val.isString())
                    {
                        ptr->setText(val.toString());
                    }
                };
                auto toArgs = [ptr = QPointer(edit),
                               id = arg.id](QJsonObject &obj) {
                    if (!ptr)
                    {
                        return;
                    }
                    obj.insert(id, ptr->text());
                };

                this->pluginControlCallbacks.emplace_back(
                    PluginControlFunctions{
                        .applyFromArguments = std::move(fromArgs),
                        .applyToArguments = std::move(toArgs),
                    });

                return edit;
            },
        },
        arg.data);
}
#endif

}  // namespace chatterino
