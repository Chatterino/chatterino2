#include "SelectChannelDialog.hpp"

#include "Application.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Theme.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/dialogs/IrcConnectionEditor.hpp"
#include "widgets/helper/NotebookTab.hpp"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include <QTableView>
#include "providers/irc/Irc2.hpp"
#include "widgets/helper/EditableModelView.hpp"

#define TAB_TWITCH 0
#define TAB_IRC 1

namespace chatterino {

SelectChannelDialog::SelectChannelDialog(QWidget *parent)
    : BaseWindow(BaseWindow::EnableCustomFrame, parent)
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
            vbox.emplace<QLabel>("Join a twitch channel by its name.").hidden();
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
                                 "chatterino is running.")
                .hidden();

        whispers_lbl->setWordWrap(true);
        whispers_btn->installEventFilter(&this->tabFilter_);

        QObject::connect(
            whispers_btn.getElement(), &QRadioButton::toggled,
            [=](bool enabled) mutable { whispers_lbl->setVisible(enabled); });

        // mentions_btn
        auto mentions_btn = vbox.emplace<QRadioButton>("Mentions")
                                .assign(&this->ui_.twitch.mentions);
        auto mentions_lbl =
            vbox.emplace<QLabel>("Shows all the messages that highlight you "
                                 "from any channel.")
                .hidden();

        mentions_lbl->setWordWrap(true);
        mentions_btn->installEventFilter(&this->tabFilter_);

        QObject::connect(
            mentions_btn.getElement(), &QRadioButton::toggled,
            [=](bool enabled) mutable { mentions_lbl->setVisible(enabled); });

        // watching_btn
        auto watching_btn = vbox.emplace<QRadioButton>("Watching")
                                .assign(&this->ui_.twitch.watching);
        auto watching_lbl =
            vbox.emplace<QLabel>("Requires the chatterino browser extension.")
                .hidden();

        watching_lbl->setWordWrap(true);
        watching_btn->installEventFilter(&this->tabFilter_);

        QObject::connect(
            watching_btn.getElement(), &QRadioButton::toggled,
            [=](bool enabled) mutable { watching_lbl->setVisible(enabled); });

        vbox->addStretch(1);

        // tabbing order
        QWidget::setTabOrder(watching_btn.getElement(),
                             channel_btn.getElement());
        QWidget::setTabOrder(channel_btn.getElement(),
                             whispers_btn.getElement());
        QWidget::setTabOrder(whispers_btn.getElement(),
                             mentions_btn.getElement());
        QWidget::setTabOrder(mentions_btn.getElement(),
                             watching_btn.getElement());

        // tab
        auto tab = notebook->addPage(obj.getElement());
        tab->setCustomTitle("Twitch");
    }

    // irc
    {
        LayoutCreator<QWidget> obj(new QWidget());
        auto outerBox = obj.setLayoutType<QFormLayout>();

        {
            auto view = this->ui_.irc.servers =
                new EditableModelView(Irc::instance().newConnectionModel(this));

            view->setTitles({"host", "port", "ssl", "user", "nick", "real",
                             "password", "login command"});
            view->getTableView()->horizontalHeader()->resizeSection(0, 140);

            view->getTableView()->horizontalHeader()->setSectionHidden(1, true);
            view->getTableView()->horizontalHeader()->setSectionHidden(2, true);
            view->getTableView()->horizontalHeader()->setSectionHidden(4, true);
            view->getTableView()->horizontalHeader()->setSectionHidden(5, true);

            view->addButtonPressed.connect([] {
                auto unique = IrcServerData{};
                unique.id = Irc::instance().uniqueId();

                auto editor = new IrcConnectionEditor(unique);
                if (editor->exec() == QDialog::Accepted)
                {
                    Irc::instance().connections.appendItem(editor->data());
                }
            });

            QObject::connect(
                view->getTableView(), &QTableView::doubleClicked,
                [](const QModelIndex &index) {
                    auto editor = new IrcConnectionEditor(
                        Irc::instance()
                            .connections.getVector()[size_t(index.row())]);

                    if (editor->exec() == QDialog::Accepted)
                    {
                        auto data = editor->data();
                        auto &&conns = Irc::instance().connections.getVector();
                        int i = 0;
                        for (auto &&conn : conns)
                        {
                            if (conn.id == data.id)
                            {
                                Irc::instance().connections.removeItem(
                                    i, Irc::noEraseCredentialCaller);
                                Irc::instance().connections.insertItem(data, i);
                            }
                            i++;
                        }
                    }
                });

            outerBox->addRow("Server:", view);
        }

        outerBox->addRow("Channel:", this->ui_.irc.channel = new QLineEdit);

        auto tab = notebook->addPage(obj.getElement());
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
        QObject::connect(button_ok, &QPushButton::clicked,
                         [=](bool) { this->ok(); });
        auto *button_cancel = buttons->addButton(QDialogButtonBox::Cancel);
        QObject::connect(button_cancel, &QAbstractButton::clicked,
                         [=](bool) { this->close(); });
    }

    this->setMinimumSize(300, 310);
    this->ui_.notebook->selectIndex(TAB_TWITCH);
    this->ui_.twitch.channel->setFocus();

    // Shortcuts
    auto *shortcut_ok = new QShortcut(QKeySequence("Return"), this);
    QObject::connect(shortcut_ok, &QShortcut::activated, [=] { this->ok(); });
    auto *shortcut_cancel = new QShortcut(QKeySequence("Esc"), this);
    QObject::connect(shortcut_cancel, &QShortcut::activated,
                     [=] { this->close(); });

    // restore ui state
    // fourtf: enable when releasing irc
    if (getSettings()->enableExperimentalIrc)
    {
        this->ui_.notebook->selectIndex(getSettings()->lastSelectChannelTab);
    }

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
        case Channel::Type::Irc: {
            this->ui_.notebook->selectIndex(TAB_IRC);
            this->ui_.irc.channel->setText(_channel.get()->getName());

            if (auto ircChannel =
                    dynamic_cast<IrcChannel *>(_channel.get().get()))
            {
                if (auto server = ircChannel->server())
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

    auto app = getApp();

    switch (this->ui_.notebook->getSelectedIndex())
    {
        case TAB_TWITCH: {
            if (this->ui_.twitch.channel->isChecked())
            {
                return app->twitch.server->getOrAddChannel(
                    this->ui_.twitch.channelName->text().trimmed());
            }
            else if (this->ui_.twitch.watching->isChecked())
            {
                return app->twitch.server->watchingChannel;
            }
            else if (this->ui_.twitch.mentions->isChecked())
            {
                return app->twitch.server->mentionsChannel;
            }
            else if (this->ui_.twitch.whispers->isChecked())
            {
                return app->twitch.server->whispersChannel;
            }
        }
        break;
        case TAB_IRC: {
            int row = this->ui_.irc.servers->getTableView()
                          ->selectionModel()
                          ->currentIndex()
                          .row();

            auto &&vector = Irc::instance().connections.getVector();

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
        widget->grabKeyboard();

        auto *radio = dynamic_cast<QRadioButton *>(watched);
        if (radio)
        {
            radio->setChecked(true);
        }

        return true;
    }
    else if (event->type() == QEvent::FocusOut)
    {
        widget->releaseKeyboard();
        return false;
    }
    else if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *event_key = static_cast<QKeyEvent *>(event);
        if ((event_key->key() == Qt::Key_Tab ||
             event_key->key() == Qt::Key_Down) &&
            event_key->modifiers() == Qt::NoModifier)
        {
            if (widget == this->dialog->ui_.twitch.channelName)
            {
                this->dialog->ui_.twitch.whispers->setFocus();
                return true;
            }
            else
            {
                widget->nextInFocusChain()->setFocus();
            }
            return true;
        }
        else if (((event_key->key() == Qt::Key_Tab ||
                   event_key->key() == Qt::Key_Backtab) &&
                  event_key->modifiers() == Qt::ShiftModifier) ||
                 ((event_key->key() == Qt::Key_Up) &&
                  event_key->modifiers() == Qt::NoModifier))
        {
            if (widget == this->dialog->ui_.twitch.channelName)
            {
                this->dialog->ui_.twitch.watching->setFocus();
                return true;
            }
            else if (widget == this->dialog->ui_.twitch.whispers)
            {
                this->dialog->ui_.twitch.channel->setFocus();
                return true;
            }

            widget->previousInFocusChain()->setFocus();
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

}  // namespace chatterino
