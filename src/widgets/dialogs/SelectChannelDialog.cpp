#include "SelectChannelDialog.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "providers/irc/Irc2.hpp"
#include "providers/irc/IrcChannel2.hpp"
#include "providers/irc/IrcServer.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/dialogs/IrcConnectionEditor.hpp"
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

#define TAB_TWITCH 0
#define TAB_IRC 1

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

    // irc
    {
        LayoutCreator<QWidget> obj(new QWidget());
        auto outerBox = obj.setLayoutType<QFormLayout>();

        {
            auto *view = this->ui_.irc.servers =
                new EditableModelView(Irc::instance().newConnectionModel(this));

            view->setTitles({"host", "port", "ssl", "user", "nick", "real",
                             "password", "login command"});
            view->getTableView()->horizontalHeader()->resizeSection(0, 140);

            view->getTableView()->horizontalHeader()->setSectionHidden(1, true);
            view->getTableView()->horizontalHeader()->setSectionHidden(2, true);
            view->getTableView()->horizontalHeader()->setSectionHidden(4, true);
            view->getTableView()->horizontalHeader()->setSectionHidden(5, true);

            // We can safely ignore this signal's connection since the button won't be
            // accessible after this dialog is closed
            std::ignore = view->addButtonPressed.connect([] {
                auto unique = IrcServerData{};
                unique.id = Irc::instance().uniqueId();

                auto *editor = new IrcConnectionEditor(unique);
                if (editor->exec() == QDialog::Accepted)
                {
                    Irc::instance().connections.append(editor->data());
                }
            });

            QObject::connect(
                view->getTableView(), &QTableView::doubleClicked,
                [](const QModelIndex &index) {
                    auto *editor = new IrcConnectionEditor(
                        Irc::instance().connections.raw()[size_t(index.row())]);

                    if (editor->exec() == QDialog::Accepted)
                    {
                        auto data = editor->data();
                        auto &&conns = Irc::instance().connections.raw();
                        int i = 0;
                        for (auto &&conn : conns)
                        {
                            if (conn.id == data.id)
                            {
                                Irc::instance().connections.removeAt(
                                    i, Irc::noEraseCredentialCaller);
                                Irc::instance().connections.insert(data, i);
                            }
                            i++;
                        }
                    }
                });

            outerBox->addRow("Server:", view);
        }

        outerBox->addRow("Channel: #", this->ui_.irc.channel = new QLineEdit);

        auto *tab = notebook->addPage(obj.getElement());
        tab->setCustomTitle("Irc (Beta)");

        if (!getSettings()->enableExperimentalIrc)
        {
            tab->setEnable(false);
            tab->setVisible(false);
        }
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

    // restore ui state
    // fourtf: enable when releasing irc
    if (getSettings()->enableExperimentalIrc)
    {
        this->ui_.notebook->selectIndex(getSettings()->lastSelectChannelTab);
    }

    this->addShortcuts();

    this->ui_.irc.servers->getTableView()->selectRow(
        getSettings()->lastSelectIrcConn);
}

void SelectChannelDialog::ok()
{
    // save ui state
    getSettings()->lastSelectChannelTab =
        this->ui_.notebook->getSelectedIndex();
    getSettings()->lastSelectIrcConn = this->ui_.irc.servers->getTableView()
                                           ->selectionModel()
                                           ->currentIndex()
                                           .row();

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
        case Channel::Type::Irc: {
            this->ui_.notebook->selectIndex(TAB_IRC);
            this->ui_.irc.channel->setText(_channel.get()->getName());

            if (auto *ircChannel =
                    dynamic_cast<IrcChannel *>(_channel.get().get()))
            {
                if (auto *server = ircChannel->server())
                {
                    int i = 0;
                    for (auto &&conn : Irc::instance().connections)
                    {
                        if (conn.id == server->id())
                        {
                            this->ui_.irc.servers->getTableView()->selectRow(i);
                            break;
                        }
                        i++;
                    }
                }
            }

            this->ui_.irc.channel->setFocus();
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

    auto *app = getApp();

    switch (this->ui_.notebook->getSelectedIndex())
    {
        case TAB_TWITCH: {
            if (this->ui_.twitch.channel->isChecked())
            {
                return app->twitch->getOrAddChannel(
                    this->ui_.twitch.channelName->text().trimmed());
            }
            else if (this->ui_.twitch.watching->isChecked())
            {
                return getIApp()->getTwitch()->getWatchingChannel();
            }
            else if (this->ui_.twitch.mentions->isChecked())
            {
                return app->twitch->mentionsChannel;
            }
            else if (this->ui_.twitch.whispers->isChecked())
            {
                return app->twitch->whispersChannel;
            }
            else if (this->ui_.twitch.live->isChecked())
            {
                return getIApp()->getTwitch()->getLiveChannel();
            }
            else if (this->ui_.twitch.automod->isChecked())
            {
                return getIApp()->getTwitch()->getAutomodChannel();
            }
        }
        break;
        case TAB_IRC: {
            int row = this->ui_.irc.servers->getTableView()
                          ->selectionModel()
                          ->currentIndex()
                          .row();

            auto &&vector = Irc::instance().connections.raw();

            if (row >= 0 && row < int(vector.size()))
            {
                return Irc::instance().getOrAddChannel(
                    vector[size_t(row)].id, this->ui_.irc.channel->text());
            }
            else
            {
                return Channel::getEmpty();
            }
        }
            //break;
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

    if (this->theme->isLightTheme())
    {
        this->setStyleSheet(
            "QRadioButton { color: #000 } QLabel { color: #000 }");
    }
    else
    {
        this->setStyleSheet(
            "QRadioButton { color: #fff } QLabel { color: #fff }");
    }
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
    };

    if (getSettings()->enableExperimentalIrc)
    {
        actions.insert(
            {"openTab", [this](std::vector<QString> arguments) -> QString {
                 if (arguments.size() == 0)
                 {
                     qCWarning(chatterinoHotkeys)
                         << "openTab shortcut called without arguments. "
                            "Takes only "
                            "one argument: tab specifier";
                     return "openTab shortcut called without arguments. "
                            "Takes only one argument: tab specifier";
                 }
                 auto target = arguments.at(0);
                 if (target == "last")
                 {
                     this->ui_.notebook->selectLastTab();
                 }
                 else if (target == "next")
                 {
                     this->ui_.notebook->selectNextTab();
                 }
                 else if (target == "previous")
                 {
                     this->ui_.notebook->selectPreviousTab();
                 }
                 else
                 {
                     bool ok;
                     int result = target.toInt(&ok);
                     if (ok)
                     {
                         this->ui_.notebook->selectIndex(result);
                     }
                     else
                     {
                         qCWarning(chatterinoHotkeys)
                             << "Invalid argument for openTab shortcut";
                         return QString("Invalid argument for openTab "
                                        "shortcut: \"%1\". Use \"last\", "
                                        "\"next\", \"previous\" or an integer.")
                             .arg(target);
                     }
                 }
                 return "";
             }});
    }
    else
    {
        actions.emplace("openTab", nullptr);
    }

    this->shortcuts_ = getIApp()->getHotkeys()->shortcutsForCategory(
        HotkeyCategory::PopupWindow, actions, this);
}

}  // namespace chatterino
