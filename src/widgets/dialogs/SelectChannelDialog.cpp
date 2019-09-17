#include "SelectChannelDialog.hpp"

#include "Application.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/Theme.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/helper/NotebookTab.hpp"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#define TAB_TWITCH 0

namespace chatterino {

SelectChannelDialog::SelectChannelDialog(QWidget *parent)
    : BaseWindow(BaseWindow::EnableCustomFrame, parent)
    , selectedChannels_{Channel::getEmpty()}
{
    this->setWindowTitle("Select channels to join");
    // TODO: Aassghdsödfkh
    this->setStyleSheet("QCheckBox:focus {"
                            "border-width: 1px;"
                            "border-style: solid;"
                            "border-color: blue;"
                        "}");

    this->tabFilter_.dialog = this;

    LayoutCreator<QWidget> layoutWidget(this->getLayoutContainer());
    auto layout = layoutWidget.setLayoutType<QVBoxLayout>().withoutMargin();
    auto notebook = layout.emplace<Notebook>(this).assign(&this->ui_.notebook);

    // twitch
    {
        LayoutCreator<QWidget> obj(new QWidget());
        auto vbox = obj.setLayoutType<QVBoxLayout>();

        // channel_btn
        auto channel_btn = vbox.emplace<QCheckBox>("Channels").assign(
            &this->ui_.twitch.channels);
        auto channel_lbl =
            vbox.emplace<QLabel>("Join twitch channels by their names.").hidden();
        channel_lbl->setWordWrap(true);
        auto channel_edit = vbox.emplace<QLineEdit>().hidden().assign(
            &this->ui_.twitch.channelNames);

        QObject::connect(channel_btn.getElement(), &QCheckBox::stateChanged,
                         [=](int enabled) mutable {
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
        auto whispers_btn = vbox.emplace<QCheckBox>("Whispers")
                                .assign(&this->ui_.twitch.whispers);
        auto whispers_lbl =
            vbox.emplace<QLabel>("Shows the whispers that you receive while "
                                 "chatterino is running.")
                .hidden();

        whispers_lbl->setWordWrap(true);
        whispers_btn->installEventFilter(&this->tabFilter_);

        QObject::connect(
            whispers_btn.getElement(), &QCheckBox::stateChanged,
            [=](int enabled) mutable { whispers_lbl->setVisible(enabled); });

        // mentions_btn
        auto mentions_btn = vbox.emplace<QCheckBox>("Mentions")
                                .assign(&this->ui_.twitch.mentions);
        auto mentions_lbl =
            vbox.emplace<QLabel>("Shows all the messages that highlight you "
                                 "from any channel.")
                .hidden();

        mentions_lbl->setWordWrap(true);
        mentions_btn->installEventFilter(&this->tabFilter_);

        QObject::connect(
            mentions_btn.getElement(), &QCheckBox::stateChanged,
            [=](int enabled) mutable { mentions_lbl->setVisible(enabled); });

        // watching_btn
        auto watching_btn = vbox.emplace<QCheckBox>("Watching")
                                .assign(&this->ui_.twitch.watching);
        auto watching_lbl =
            vbox.emplace<QLabel>("Requires the chatterino browser extension.")
                .hidden();

        watching_lbl->setWordWrap(true);
        watching_btn->installEventFilter(&this->tabFilter_);

        QObject::connect(
            watching_btn.getElement(), &QCheckBox::stateChanged,
            [=](int enabled) mutable { watching_lbl->setVisible(enabled); });

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

        channel_btn->setChecked(true);

        // tab
        auto tab = notebook->addPage(obj.getElement());
        tab->setCustomTitle("Twitch");
    }

    // irc
    /*
        {
            LayoutCreator<QWidget> obj(new QWidget());
            auto vbox = obj.setLayoutType<QVBoxLayout>();
            auto form = vbox.emplace<QFormLayout>();

            form->addRow(new QLabel("User name:"), new QLineEdit());
            form->addRow(new QLabel("First nick choice:"), new QLineEdit());
            form->addRow(new QLabel("Second nick choice:"), new QLineEdit());
            form->addRow(new QLabel("Third nick choice:"), new QLineEdit());

            auto tab = notebook->addPage(obj.getElement());
            tab->setCustomTitle("Irc");
        }
    */

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

    this->setScaleIndependantSize(300, 310);
    this->ui_.notebook->selectIndex(TAB_TWITCH);
    this->ui_.twitch.channels->setFocus();

    // TODO: What to do about these?
    // Shortcuts
    // auto *shortcut_ok = new QShortcut(QKeySequence("Return"), this);
    // QObject::connect(shortcut_ok, &QShortcut::activated, [=] { this->ok(); });
    auto *shortcut_cancel = new QShortcut(QKeySequence("Esc"), this);
    QObject::connect(shortcut_cancel, &QShortcut::activated,
                     [=] { this->close(); });
}

void SelectChannelDialog::ok()
{
    this->hasSelectedChannels_ = true;
    this->close();
}

void SelectChannelDialog::setSelectedChannels(
    std::vector<IndirectChannel> _channels)
{
    this->selectedChannels_ = _channels;

    // TODO: What does this do? / Is this needed?
    for (auto channel : _channels)
    {
        assert(channel.get());

        switch (channel.getType())
        {
            case Channel::Type::Twitch:
            {
                this->ui_.notebook->selectIndex(TAB_TWITCH);
                this->ui_.twitch.channels->setFocus();
                this->ui_.twitch.channelNames->setText(channel.get()->getName());
            }
            break;
            case Channel::Type::TwitchWatching:
            {
                this->ui_.notebook->selectIndex(TAB_TWITCH);
                this->ui_.twitch.watching->setFocus();
            }
            break;
            case Channel::Type::TwitchMentions:
            {
                this->ui_.notebook->selectIndex(TAB_TWITCH);
                this->ui_.twitch.mentions->setFocus();
            }
            break;
            case Channel::Type::TwitchWhispers:
            {
                this->ui_.notebook->selectIndex(TAB_TWITCH);
                this->ui_.twitch.whispers->setFocus();
            }
            break;
            default:
            {
                this->ui_.notebook->selectIndex(TAB_TWITCH);
                this->ui_.twitch.channels->setFocus();
            }
        }
    }

    this->hasSelectedChannels_ = false;
}

std::vector<IndirectChannel> SelectChannelDialog::getSelectedChannels() const
{
    if (!this->hasSelectedChannels_)
    {
        return this->selectedChannels_;
    }

    auto app = getApp();

    std::vector<IndirectChannel> channels;
    switch (this->ui_.notebook->getSelectedIndex())
    {
        case TAB_TWITCH:
        {
            if (this->ui_.twitch.channels->checkState())
            {
                QStringList channelNames =
                    this->ui_.twitch.channelNames->text().split(' ', QString::SkipEmptyParts);

                for (const QString &channelName : channelNames)
                {
                    channels.emplace_back(
                        app->twitch.server->getOrAddChannel(channelName),
                        Channel::Type::Twitch);
                }
            }

            if (this->ui_.twitch.watching->checkState())
            {
                channels.push_back(app->twitch.server->watchingChannel);
            }

            if (this->ui_.twitch.mentions->checkState())
            {
                channels.emplace_back(app->twitch.server->mentionsChannel,
                                      Channel::Type::TwitchMentions);
            }

            if (this->ui_.twitch.whispers->checkState())
            {
                channels.emplace_back(app->twitch.server->whispersChannel,
                                      Channel::Type::TwitchWhispers);
            }
        }

            return channels;
    }

    return this->selectedChannels_;
}

bool SelectChannelDialog::hasSelectedChannels() const
{
    return this->hasSelectedChannels_;
}

bool SelectChannelDialog::EventFilter::eventFilter(QObject *watched,
                                                   QEvent *event)
{
    auto *widget = (QWidget *)watched;

    if (event->type() == QEvent::FocusIn)
    {
        if (widget == this->dialog->ui_.twitch.channels)
        {
            this->dialog->ui_.twitch.channelNames->grabKeyboard();
            this->dialog->ui_.twitch.channelNames->setFocus();
        }
        else
        {
            widget->grabKeyboard();
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
            if (widget == this->dialog->ui_.twitch.channelNames)
            {
                this->dialog->ui_.twitch.whispers->setFocus();
                return true;
            }
            else if (widget == this->dialog->ui_.twitch.watching)
            {
                this->dialog->ui_.twitch.channels->setFocus();
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
            if (widget == this->dialog->ui_.twitch.channelNames)
            {
                this->dialog->ui_.twitch.watching->setFocus();
                return true;
            }
            else if (widget == this->dialog->ui_.twitch.whispers)
            {
                this->dialog->ui_.twitch.channels->setFocus();
                return true;
            }

            widget->previousInFocusChain()->setFocus();
            return true;
        }
        else if ((event_key->key() == Qt::Key_Enter || event_key->key() == Qt::Key_Return) && (event_key->modifiers() == Qt::NoModifier))
        {
            // When the Enter key is pressed, check boxes are toggled.
            // If the channelNames QLineEdit has the focus, we check whether
            // the field is empty.
            // If it is empty, the corresponding check box is toggled.
            // If it is not empty (= the user has entered channel names), the
            // dialog is ok()'d instead.

            if (auto *checkBox = dynamic_cast<QCheckBox *>(watched))
            {
                checkBox->toggle();
            }
            else if (auto *lineEdit = dynamic_cast<QLineEdit *>(watched))
            {
                if (!lineEdit->text().isEmpty())
                {
                    this->dialog->ok();
                }
                else
                {
                    this->dialog->ui_.twitch.channels->toggle();
                    this->dialog->ui_.twitch.channels->setFocus();
                }
            }

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
            "QCheckBox { color: #000 } QLabel { color: #000 }");
    }
    else
    {
        this->setStyleSheet(
            "QCheckBox { color: #fff } QLabel { color: #fff }");
    }
}

}  // namespace chatterino
