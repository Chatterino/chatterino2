#include "widgets/splits/SplitInput.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "messages/Link.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/Clamp.hpp"
#include "util/Helpers.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Scrollbar.hpp"
#include "widgets/dialogs/EmotePopup.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/EffectLabel.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"
#include "widgets/splits/InputCompletionPopup.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"
#include "widgets/splits/SplitInput.hpp"

#include <QCompleter>
#include <QPainter>

namespace chatterino {
const int TWITCH_MESSAGE_LIMIT = 500;

SplitInput::SplitInput(Split *_chatWidget)
    : BaseWidget(_chatWidget)
    , split_(_chatWidget)
{
    this->installEventFilter(this);
    this->initLayout();

    auto completer =
        new QCompleter(&this->split_->getChannel().get()->completionModel);
    this->ui_.textEdit->setCompleter(completer);

    this->split_->channelChanged.connect([this] {
        auto completer =
            new QCompleter(&this->split_->getChannel()->completionModel);
        this->ui_.textEdit->setCompleter(completer);
    });

    // misc
    this->installKeyPressedEvent();
    this->addShortcuts();
    this->ui_.textEdit->focusLost.connect([this] {
        this->hideCompletionPopup();
    });
    this->scaleChangedEvent(this->scale());
    this->signalHolder_.managedConnect(getApp()->hotkeys->onItemsUpdated,
                                       [this]() {
                                           this->clearShortcuts();
                                           this->addShortcuts();
                                       });
}

void SplitInput::initLayout()
{
    auto app = getApp();
    LayoutCreator<SplitInput> layoutCreator(this);

    auto layout =
        layoutCreator.setLayoutType<QHBoxLayout>().withoutMargin().assign(
            &this->ui_.hbox);

    // input
    auto textEdit =
        layout.emplace<ResizingTextEdit>().assign(&this->ui_.textEdit);
    connect(textEdit.getElement(), &ResizingTextEdit::textChanged, this,
            &SplitInput::editTextChanged);

    // right box
    auto box = layout.emplace<QVBoxLayout>().withoutMargin();
    box->setSpacing(0);
    {
        auto textEditLength =
            box.emplace<QLabel>().assign(&this->ui_.textEditLength);
        textEditLength->setAlignment(Qt::AlignRight);

        box->addStretch(1);
        box.emplace<EffectLabel>().assign(&this->ui_.emoteButton);
    }

    this->ui_.emoteButton->getLabel().setTextFormat(Qt::RichText);

    // ---- misc

    // set edit font
    this->ui_.textEdit->setFont(
        app->fonts->getFont(FontStyle::ChatMedium, this->scale()));
    QObject::connect(this->ui_.textEdit, &QTextEdit::cursorPositionChanged,
                     this, &SplitInput::onCursorPositionChanged);
    QObject::connect(this->ui_.textEdit, &QTextEdit::textChanged, this,
                     &SplitInput::onTextChanged);

    this->managedConnections_.managedConnect(app->fonts->fontChanged, [=]() {
        this->ui_.textEdit->setFont(
            app->fonts->getFont(FontStyle::ChatMedium, this->scale()));
    });

    // open emote popup
    QObject::connect(this->ui_.emoteButton, &EffectLabel::leftClicked, [=] {
        this->openEmotePopup();
    });

    // clear channelview selection when selecting in the input
    QObject::connect(this->ui_.textEdit, &QTextEdit::copyAvailable,
                     [this](bool available) {
                         if (available)
                         {
                             this->split_->view_->clearSelection();
                         }
                     });

    // textEditLength visibility
    getSettings()->showMessageLength.connect(
        [this](const bool &value, auto) {
            // this->ui_.textEditLength->setHidden(!value);
            this->editTextChanged();
        },
        this->managedConnections_);
}

void SplitInput::scaleChangedEvent(float scale)
{
    // update the icon size of the emote button
    this->updateEmoteButton();

    // set maximum height
    this->setMaximumHeight(int(150 * this->scale()));
    this->ui_.textEdit->setFont(
        getApp()->fonts->getFont(FontStyle::ChatMedium, this->scale()));
}

void SplitInput::themeChangedEvent()
{
    QPalette palette, placeholderPalette;

    palette.setColor(QPalette::WindowText, this->theme->splits.input.text);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    placeholderPalette.setColor(
        QPalette::PlaceholderText,
        this->theme->messages.textColors.chatPlaceholder);
#endif

    this->updateEmoteButton();
    this->ui_.textEditLength->setPalette(palette);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    this->ui_.textEdit->setPalette(placeholderPalette);
#endif
    this->ui_.textEdit->setStyleSheet(this->theme->splits.input.styleSheet);

    this->ui_.hbox->setMargin(
        int((this->theme->isLightTheme() ? 4 : 2) * this->scale()));

    this->ui_.emoteButton->getLabel().setStyleSheet("color: #000");
}

void SplitInput::updateEmoteButton()
{
    float scale = this->scale();

    QString text = "<img src=':/buttons/emote.svg' width='xD' height='xD' />";
    text.replace("xD", QString::number(int(12 * scale)));

    if (this->theme->isLightTheme())
    {
        text.replace("emote", "emoteDark");
    }

    this->ui_.emoteButton->getLabel().setText(text);
    this->ui_.emoteButton->setFixedHeight(int(18 * scale));
}

void SplitInput::openEmotePopup()
{
    if (!this->emotePopup_)
    {
        this->emotePopup_ = new EmotePopup(this);
        this->emotePopup_->setAttribute(Qt::WA_DeleteOnClose);

        this->emotePopup_->linkClicked.connect([this](const Link &link) {
            if (link.type == Link::InsertText)
            {
                QTextCursor cursor = this->ui_.textEdit->textCursor();
                QString textToInsert(link.value + " ");

                // If symbol before cursor isn't space or empty
                // Then insert space before emote.
                if (cursor.position() > 0 &&
                    !this->getInputText()[cursor.position() - 1].isSpace())
                {
                    textToInsert = " " + textToInsert;
                }
                this->insertText(textToInsert);
            }
        });
    }

    this->emotePopup_->resize(int(300 * this->emotePopup_->scale()),
                              int(500 * this->emotePopup_->scale()));
    this->emotePopup_->loadChannel(this->split_->getChannel());
    this->emotePopup_->show();
    this->emotePopup_->activateWindow();
}

void SplitInput::addShortcuts()
{
    HotkeyController::HotkeyMap actions{
        {"cursorToStart",
         [this](std::vector<QString> arguments) -> QString {
             if (arguments.size() != 1)
             {
                 qCWarning(chatterinoHotkeys)
                     << "Invalid cursorToStart arguments. Argument 0: select "
                        "(\"withSelection\" or \"withoutSelection\")";
                 return "Invalid cursorToStart arguments. Argument 0: select "
                        "(\"withSelection\" or \"withoutSelection\")";
             }
             QTextCursor cursor = this->ui_.textEdit->textCursor();
             auto place = QTextCursor::Start;
             auto stringTakeSelection = arguments.at(0);
             bool select;
             if (stringTakeSelection == "withSelection")
             {
                 select = true;
             }
             else if (stringTakeSelection == "withoutSelection")
             {
                 select = false;
             }
             else
             {
                 qCWarning(chatterinoHotkeys)
                     << "Invalid cursorToStart select argument (0)!";
                 return "Invalid cursorToStart select argument (0)!";
             }

             cursor.movePosition(place,
                                 select ? QTextCursor::MoveMode::KeepAnchor
                                        : QTextCursor::MoveMode::MoveAnchor);
             this->ui_.textEdit->setTextCursor(cursor);
             return "";
         }},
        {"cursorToEnd",
         [this](std::vector<QString> arguments) -> QString {
             if (arguments.size() != 1)
             {
                 qCWarning(chatterinoHotkeys)
                     << "Invalid cursorToEnd arguments. Argument 0: select "
                        "(\"withSelection\" or \"withoutSelection\")";
                 return "Invalid cursorToEnd arguments. Argument 0: select "
                        "(\"withSelection\" or \"withoutSelection\")";
             }
             QTextCursor cursor = this->ui_.textEdit->textCursor();
             auto place = QTextCursor::End;
             auto stringTakeSelection = arguments.at(0);
             bool select;
             if (stringTakeSelection == "withSelection")
             {
                 select = true;
             }
             else if (stringTakeSelection == "withoutSelection")
             {
                 select = false;
             }
             else
             {
                 qCWarning(chatterinoHotkeys)
                     << "Invalid cursorToEnd select argument (0)!";
                 return "Invalid cursorToEnd select argument (0)!";
             }

             cursor.movePosition(place,
                                 select ? QTextCursor::MoveMode::KeepAnchor
                                        : QTextCursor::MoveMode::MoveAnchor);
             this->ui_.textEdit->setTextCursor(cursor);
             return "";
         }},
        {"openEmotesPopup",
         [this](std::vector<QString>) -> QString {
             this->openEmotePopup();
             return "";
         }},
        {"sendMessage",
         [this](std::vector<QString> arguments) -> QString {
             auto c = this->split_->getChannel();
             if (c == nullptr)
                 return "";

             QString message = ui_.textEdit->toPlainText();

             message = message.replace('\n', ' ');
             QString sendMessage =
                 getApp()->commands->execCommand(message, c, false);

             c->sendMessage(sendMessage);
             // don't add duplicate messages and empty message to message history
             if ((this->prevMsg_.isEmpty() ||
                  !this->prevMsg_.endsWith(message)) &&
                 !message.trimmed().isEmpty())
             {
                 this->prevMsg_.append(message);
             }
             bool shouldClearInput = true;
             if (arguments.size() != 0 && arguments.at(0) == "keepInput")
             {
                 shouldClearInput = false;
             }

             if (shouldClearInput)
             {
                 this->currMsg_ = QString();
                 this->ui_.textEdit->setPlainText(QString());
             }
             this->prevIndex_ = this->prevMsg_.size();
             return "";
         }},
        {"previousMessage",
         [this](std::vector<QString>) -> QString {
             if (this->prevMsg_.size() && this->prevIndex_)
             {
                 if (this->prevIndex_ == (this->prevMsg_.size()))
                 {
                     this->currMsg_ = ui_.textEdit->toPlainText();
                 }

                 this->prevIndex_--;
                 this->ui_.textEdit->setPlainText(
                     this->prevMsg_.at(this->prevIndex_));

                 QTextCursor cursor = this->ui_.textEdit->textCursor();
                 cursor.movePosition(QTextCursor::End);
                 this->ui_.textEdit->setTextCursor(cursor);
             }
             return "";
         }},
        {"nextMessage",
         [this](std::vector<QString>) -> QString {
             // If user did not write anything before then just do nothing.
             if (this->prevMsg_.isEmpty())
             {
                 return "";
             }
             bool cursorToEnd = true;
             QString message = ui_.textEdit->toPlainText();

             if (this->prevIndex_ != (this->prevMsg_.size() - 1) &&
                 this->prevIndex_ != this->prevMsg_.size())
             {
                 this->prevIndex_++;
                 this->ui_.textEdit->setPlainText(
                     this->prevMsg_.at(this->prevIndex_));
             }
             else
             {
                 this->prevIndex_ = this->prevMsg_.size();
                 if (message == this->prevMsg_.at(this->prevIndex_ - 1))
                 {
                     // If user has just come from a message history
                     // Then simply get currMsg_.
                     this->ui_.textEdit->setPlainText(this->currMsg_);
                 }
                 else if (message != this->currMsg_)
                 {
                     // If user are already in current message
                     // And type something new
                     // Then replace currMsg_ with new one.
                     this->currMsg_ = message;
                 }
                 // If user is already in current message
                 // Then don't touch cursos.
                 cursorToEnd =
                     (message == this->prevMsg_.at(this->prevIndex_ - 1));
             }

             if (cursorToEnd)
             {
                 QTextCursor cursor = this->ui_.textEdit->textCursor();
                 cursor.movePosition(QTextCursor::End);
                 this->ui_.textEdit->setTextCursor(cursor);
             }
             return "";
         }},
        {"undo",
         [this](std::vector<QString>) -> QString {
             this->ui_.textEdit->undo();
             return "";
         }},
        {"redo",
         [this](std::vector<QString>) -> QString {
             this->ui_.textEdit->redo();
             return "";
         }},
        {"copy",
         [this](std::vector<QString> arguments) -> QString {
             // XXX: this action is unused at the moment, a qt standard shortcut is used instead
             if (arguments.size() == 0)
             {
                 return "copy action takes only one argument: the source "
                        "of the copy \"split\", \"input\" or "
                        "\"auto\". If the source is \"split\", only text "
                        "from the chat will be copied. If it is "
                        "\"splitInput\", text from the input box will be "
                        "copied. Automatic will pick whichever has a "
                        "selection";
             }
             bool copyFromSplit = false;
             auto mode = arguments.at(0);
             if (mode == "split")
             {
                 copyFromSplit = true;
             }
             else if (mode == "splitInput")
             {
                 copyFromSplit = false;
             }
             else if (mode == "auto")
             {
                 const auto &cursor = this->ui_.textEdit->textCursor();
                 copyFromSplit = !cursor.hasSelection();
             }

             if (copyFromSplit)
             {
                 this->split_->copyToClipboard();
             }
             else
             {
                 this->ui_.textEdit->copy();
             }
             return "";
         }},
        {"paste",
         [this](std::vector<QString>) -> QString {
             this->ui_.textEdit->paste();
             return "";
         }},
        {"clear",
         [this](std::vector<QString>) -> QString {
             this->ui_.textEdit->setText("");
             this->ui_.textEdit->moveCursor(QTextCursor::Start);
             return "";
         }},
        {"selectAll",
         [this](std::vector<QString>) -> QString {
             this->ui_.textEdit->selectAll();
             return "";
         }},
    };

    this->shortcuts_ = getApp()->hotkeys->shortcutsForCategory(
        HotkeyCategory::SplitInput, actions, this->parentWidget());
}

bool SplitInput::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride ||
        event->type() == QEvent::Shortcut)
    {
        if (auto popup = this->inputCompletionPopup_.get())
        {
            if (popup->isVisible())
            {
                // Stop shortcut from triggering by saying we will handle it ourselves
                event->accept();

                // Return false means the underlying event isn't stopped, it will continue to propagate
                return false;
            }
        }
    }

    return BaseWidget::eventFilter(obj, event);
}

void SplitInput::installKeyPressedEvent()
{
    this->ui_.textEdit->keyPressed.disconnectAll();
    this->ui_.textEdit->keyPressed.connect([this](QKeyEvent *event) {
        if (auto popup = this->inputCompletionPopup_.get())
        {
            if (popup->isVisible())
            {
                if (popup->eventFilter(nullptr, event))
                {
                    event->accept();
                    return;
                }
            }
        }

        // One of the last remaining of it's kind, the copy shortcut.
        // For some bizarre reason Qt doesn't want this key be rebound.
        // TODO(Mm2PL): Revisit in Qt6, maybe something changed?
        if ((event->key() == Qt::Key_C || event->key() == Qt::Key_Insert) &&
            event->modifiers() == Qt::ControlModifier)
        {
            if (this->split_->view_->hasSelection())
            {
                this->split_->copyToClipboard();
                event->accept();
            }
        }
    });
}

void SplitInput::onTextChanged()
{
    this->updateCompletionPopup();
}

void SplitInput::onCursorPositionChanged()
{
    this->updateCompletionPopup();
}

void SplitInput::updateCompletionPopup()
{
    auto channel = this->split_->getChannel().get();
    auto tc = dynamic_cast<TwitchChannel *>(channel);
    bool showEmoteCompletion =
        channel->isTwitchChannel() && getSettings()->emoteCompletionWithColon;
    bool showUsernameCompletion =
        tc && getSettings()->showUsernameCompletionMenu;
    if (!showEmoteCompletion && !showUsernameCompletion)
    {
        this->hideCompletionPopup();
        return;
    }

    // check if in completion prefix
    auto &edit = *this->ui_.textEdit;

    auto text = edit.toPlainText();
    auto position = edit.textCursor().position() - 1;

    if (text.length() == 0 || position == -1)
    {
        this->hideCompletionPopup();
        return;
    }

    for (int i = clamp(position, 0, text.length() - 1); i >= 0; i--)
    {
        if (text[i] == ' ')
        {
            this->hideCompletionPopup();
            return;
        }
        else if (text[i] == ':' && showEmoteCompletion)
        {
            if (i == 0 || text[i - 1].isSpace())
                this->showCompletionPopup(text.mid(i, position - i + 1).mid(1),
                                          true);
            else
                this->hideCompletionPopup();
            return;
        }
        else if (text[i] == '@' && showUsernameCompletion)
        {
            if (i == 0 || text[i - 1].isSpace())
                this->showCompletionPopup(text.mid(i, position - i + 1).mid(1),
                                          false);
            else
                this->hideCompletionPopup();
            return;
        }
    }

    this->hideCompletionPopup();
}

void SplitInput::showCompletionPopup(const QString &text, bool emoteCompletion)
{
    if (!this->inputCompletionPopup_.get())
    {
        this->inputCompletionPopup_ = new InputCompletionPopup(this);
        this->inputCompletionPopup_->setInputAction(
            [that = QObjectRef(this)](const QString &text) mutable {
                if (auto this2 = that.get())
                {
                    this2->insertCompletionText(text);
                    this2->hideCompletionPopup();
                }
            });
    }

    auto popup = this->inputCompletionPopup_.get();
    assert(popup);

    if (emoteCompletion)  // autocomplete emotes
        popup->updateEmotes(text, this->split_->getChannel());
    else  // autocomplete usernames
        popup->updateUsers(text, this->split_->getChannel());

    auto pos = this->mapToGlobal({0, 0}) - QPoint(0, popup->height()) +
               QPoint((this->width() - popup->width()) / 2, 0);

    popup->move(pos);
    popup->show();
}

void SplitInput::hideCompletionPopup()
{
    if (auto popup = this->inputCompletionPopup_.get())
        popup->hide();
}

void SplitInput::insertCompletionText(const QString &input_)
{
    auto &edit = *this->ui_.textEdit;
    auto input = input_ + ' ';

    auto text = edit.toPlainText();
    auto position = edit.textCursor().position() - 1;

    for (int i = clamp(position, 0, text.length() - 1); i >= 0; i--)
    {
        bool done = false;
        if (text[i] == ':')
        {
            done = true;
        }
        else if (text[i] == '@')
        {
            const auto userMention =
                formatUserMention(input_, edit.isFirstWord(),
                                  getSettings()->mentionUsersWithComma);
            input = "@" + userMention + " ";
            done = true;
        }

        if (done)
        {
            auto cursor = edit.textCursor();
            edit.setText(text.remove(i, position - i + 1).insert(i, input));

            cursor.setPosition(i + input.size());
            edit.setTextCursor(cursor);
            break;
        }
    }
}

void SplitInput::clearSelection()
{
    QTextCursor c = this->ui_.textEdit->textCursor();

    c.setPosition(c.position());
    c.setPosition(c.position(), QTextCursor::KeepAnchor);

    this->ui_.textEdit->setTextCursor(c);
}

bool SplitInput::isEditFirstWord() const
{
    return this->ui_.textEdit->isFirstWord();
}

QString SplitInput::getInputText() const
{
    return this->ui_.textEdit->toPlainText();
}

void SplitInput::insertText(const QString &text)
{
    this->ui_.textEdit->insertPlainText(text);
}

void SplitInput::editTextChanged()
{
    auto app = getApp();

    // set textLengthLabel value
    QString text = this->ui_.textEdit->toPlainText();

    if (text.startsWith("/r ", Qt::CaseInsensitive) &&
        this->split_->getChannel()->isTwitchChannel())
    {
        QString lastUser = app->twitch.server->lastUserThatWhisperedMe.get();
        if (!lastUser.isEmpty())
        {
            this->ui_.textEdit->setPlainText("/w " + lastUser + text.mid(2));
            this->ui_.textEdit->moveCursor(QTextCursor::EndOfBlock);
        }
    }
    else
    {
        this->textChanged.invoke(text);

        text = text.trimmed();
        text =
            app->commands->execCommand(text, this->split_->getChannel(), true);
    }

    QString labelText;

    if (text.length() > 0 && getSettings()->showMessageLength)
    {
        labelText = QString::number(text.length());
        if (text.length() > TWITCH_MESSAGE_LIMIT)
        {
            this->ui_.textEditLength->setStyleSheet("color: red");
        }
        else
        {
            this->ui_.textEditLength->setStyleSheet("");
        }
    }
    else
    {
        labelText = "";
    }

    this->ui_.textEditLength->setText(labelText);
}

void SplitInput::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (this->theme->isLightTheme())
    {
        int s = int(3 * this->scale());
        QRect rect = this->rect().marginsRemoved(QMargins(s - 1, s - 1, s, s));

        painter.fillRect(rect, this->theme->splits.input.background);

        painter.setPen(QColor("#ccc"));
        painter.drawRect(rect);
    }
    else
    {
        int s = int(1 * this->scale());
        QRect rect = this->rect().marginsRemoved(QMargins(s - 1, s - 1, s, s));

        painter.fillRect(rect, this->theme->splits.input.background);

        painter.setPen(QColor("#333"));
        painter.drawRect(rect);
    }

    //    int offset = 2;
    //    painter.fillRect(offset, this->height() - offset, this->width() - 2 *
    //    offset, 1,
    //                     getApp()->themes->splits.input.focusedLine);
}

void SplitInput::resizeEvent(QResizeEvent *)
{
    if (this->height() == this->maximumHeight())
    {
        this->ui_.textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
    else
    {
        this->ui_.textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
}

void SplitInput::mousePressEvent(QMouseEvent *)
{
    this->split_->giveFocus(Qt::MouseFocusReason);
}

}  // namespace chatterino
