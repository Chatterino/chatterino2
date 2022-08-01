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

#include <functional>

#include <QCompleter>
#include <QPainter>

namespace chatterino {
const int TWITCH_MESSAGE_LIMIT = 500;

SplitInput::SplitInput(Split *_chatWidget, bool enableInlineReplying)
    : SplitInput(_chatWidget, _chatWidget, enableInlineReplying)
{
}

SplitInput::SplitInput(QWidget *parent, Split *_chatWidget,
                       bool enableInlineReplying)
    : BaseWidget(parent)
    , split_(_chatWidget)
    , enableInlineReplying_(enableInlineReplying)
{
    this->installEventFilter(this);
    this->initLayout();

    auto completer =
        new QCompleter(&this->split_->getChannel().get()->completionModel);
    this->ui_.textEdit->setCompleter(completer);

    this->signalHolder_.managedConnect(this->split_->channelChanged, [this] {
        auto channel = this->split_->getChannel();
        auto completer = new QCompleter(&channel->completionModel);
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
        layoutCreator.setLayoutType<QVBoxLayout>().withoutMargin().assign(
            &this->ui_.vbox);

    // reply label stuff
    auto replyWrapper =
        layout.emplace<QWidget>().assign(&this->ui_.replyWrapper);
    this->ui_.replyWrapper->setContentsMargins(0, 0, 0, 0);

    auto replyHbox = replyWrapper.emplace<QHBoxLayout>().withoutMargin().assign(
        &this->ui_.replyHbox);

    auto replyLabel = replyHbox.emplace<QLabel>().assign(&this->ui_.replyLabel);
    replyLabel->setAlignment(Qt::AlignLeft);
    replyLabel->setFont(
        app->fonts->getFont(FontStyle::ChatMedium, this->scale()));

    replyHbox->addStretch(1);

    auto replyCancelButton = replyHbox.emplace<EffectLabel>(nullptr, 4)
                                 .assign(&this->ui_.cancelReplyButton);
    replyCancelButton->getLabel().setTextFormat(Qt::RichText);

    replyCancelButton->hide();
    replyLabel->hide();

    // hbox for input, right box
    auto hboxLayout =
        layout.emplace<QHBoxLayout>().withoutMargin().assign(&this->ui_.hbox);

    // input
    auto textEdit =
        hboxLayout.emplace<ResizingTextEdit>().assign(&this->ui_.textEdit);
    connect(textEdit.getElement(), &ResizingTextEdit::textChanged, this,
            &SplitInput::editTextChanged);

    // right box
    auto box = hboxLayout.emplace<QVBoxLayout>().withoutMargin();
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
        this->ui_.replyLabel->setFont(
            app->fonts->getFont(FontStyle::ChatMediumBold, this->scale()));
    });

    // open emote popup
    QObject::connect(this->ui_.emoteButton, &EffectLabel::leftClicked, [=] {
        this->openEmotePopup();
    });

    // clear input and remove reply thread
    QObject::connect(this->ui_.cancelReplyButton, &EffectLabel::leftClicked,
                     [=] {
                         this->clearInput();
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
    auto app = getApp();
    // update the icon size of the buttons
    this->updateEmoteButton();
    this->updateCancelReplyButton();

    // set maximum height
    if (!this->hidden)
    {
        this->setMaximumHeight(this->scaledMaxHeight());
    }
    this->ui_.textEdit->setFont(
        app->fonts->getFont(FontStyle::ChatMedium, this->scale()));
    this->ui_.textEditLength->setFont(
        app->fonts->getFont(FontStyle::ChatMedium, this->scale()));
    this->ui_.replyLabel->setFont(
        app->fonts->getFont(FontStyle::ChatMediumBold, this->scale()));
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
    this->updateCancelReplyButton();
    this->ui_.textEditLength->setPalette(palette);

    this->ui_.textEdit->setStyleSheet(this->theme->splits.input.styleSheet);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    this->ui_.textEdit->setPalette(placeholderPalette);
#endif

    this->ui_.vbox->setMargin(
        int((this->theme->isLightTheme() ? 4 : 2) * this->scale()));

    this->ui_.emoteButton->getLabel().setStyleSheet("color: #000");

    if (this->theme->isLightTheme())
    {
        this->ui_.replyLabel->setStyleSheet("color: #333");
    }
    else
    {
        this->ui_.replyLabel->setStyleSheet("color: #ccc");
    }
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

void SplitInput::updateCancelReplyButton()
{
    float scale = this->scale();

    QString text =
        QStringLiteral(
            "<img src=':/buttons/cancel.svg' width='%1' height='%1' />")
            .arg(QString::number(int(12 * scale)));

    if (this->theme->isLightTheme())
    {
        text.replace("cancel", "cancelDark");
    }

    this->ui_.cancelReplyButton->getLabel().setText(text);
    this->ui_.cancelReplyButton->setFixedHeight(int(12 * scale));
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
    this->emotePopup_->raise();
    this->emotePopup_->activateWindow();
}

QString SplitInput::handleSendMessage(std::vector<QString> &arguments)
{
    auto c = this->split_->getChannel();
    if (c == nullptr)
        return "";

    if (!c->isTwitchChannel() || this->replyThread_ == nullptr)
    {
        // standard message send behavior
        QString message = ui_.textEdit->toPlainText();

        message = message.replace('\n', ' ');
        QString sendMessage =
            getApp()->commands->execCommand(message, c, false);

        c->sendMessage(sendMessage);

        this->postMessageSend(message, arguments);
        return "";
    }
    else
    {
        // Reply to message
        auto tc = dynamic_cast<TwitchChannel *>(c.get());
        if (!tc)
        {
            // this should not fail
            return "";
        }

        QString message = this->ui_.textEdit->toPlainText();

        if (this->enableInlineReplying_)
        {
            // Remove @username prefix that is inserted when doing inline replies
            message.remove(0, this->replyThread_->root()->displayName.length() +
                                  1);  // remove "@username"

            if (!message.isEmpty() && message.at(0) == ' ')
            {
                message.remove(0, 1);  // remove possible space
            }
        }

        message = message.replace('\n', ' ');
        QString sendMessage =
            getApp()->commands->execCommand(message, c, false);

        // Reply within TwitchChannel
        tc->sendReply(sendMessage, this->replyThread_->rootId());

        this->postMessageSend(message, arguments);
        return "";
    }
}

void SplitInput::postMessageSend(const QString &message,
                                 const std::vector<QString> &arguments)
{
    // don't add duplicate messages and empty message to message history
    if ((this->prevMsg_.isEmpty() || !this->prevMsg_.endsWith(message)) &&
        !message.trimmed().isEmpty())
    {
        this->prevMsg_.append(message);
    }

    if (arguments.empty() || arguments.at(0) != "keepInput")
    {
        this->clearInput();
    }
    this->prevIndex_ = this->prevMsg_.size();
}

int SplitInput::scaledMaxHeight() const
{
    return int(150 * this->scale());
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
             return this->handleSendMessage(arguments);
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
             this->clearInput();
             return "";
         }},
        {"selectAll",
         [this](std::vector<QString>) -> QString {
             this->ui_.textEdit->selectAll();
             return "";
         }},
        {"selectWord",
         [this](std::vector<QString>) -> QString {
             auto cursor = this->ui_.textEdit->textCursor();
             cursor.select(QTextCursor::WordUnderCursor);
             this->ui_.textEdit->setTextCursor(cursor);
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
                                  getSettings()->mentionUsersWithComma,
                                  getSettings()->lowercaseUsernames);
            input = "@" + userMention + " ";
            done = true;
        }

        if (done)
        {
            auto cursor = edit.textCursor();
            edit.setPlainText(
                text.remove(i, position - i + 1).insert(i, input));

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

void SplitInput::hide()
{
    if (this->isHidden())
    {
        return;
    }

    this->hidden = true;
    this->setMaximumHeight(0);
    this->updateGeometry();
}

void SplitInput::show()
{
    if (!this->isHidden())
    {
        return;
    }

    this->hidden = false;
    this->setMaximumHeight(this->scaledMaxHeight());
    this->updateGeometry();
}

bool SplitInput::isHidden() const
{
    return this->hidden;
}

void SplitInput::editTextChanged()
{
    auto app = getApp();

    // set textLengthLabel value
    QString text = this->ui_.textEdit->toPlainText();

    if (text.startsWith("/r ", Qt::CaseInsensitive) &&
        this->split_->getChannel()->isTwitchChannel())
    {
        QString lastUser = app->twitch->lastUserThatWhisperedMe.get();
        if (!lastUser.isEmpty())
        {
            this->ui_.textEdit->setPlainText("/w " + lastUser + text.mid(2));
            this->ui_.textEdit->moveCursor(QTextCursor::EndOfBlock);
        }
    }
    else if (text.startsWith("/$", Qt::CaseInsensitive) &&
             this->split_->getChannel()->isTwitchChannel())  //
    {
        this->ui_.textEdit->setPlainText("/w supibot " + text.mid(1));
        this->ui_.textEdit->moveCursor(QTextCursor::EndOfBlock);
    }
    else if (text.startsWith("/_", Qt::CaseInsensitive) &&
             this->split_->getChannel()->isTwitchChannel())  //
    {
        this->ui_.textEdit->setPlainText("/w mm_sutilitybot " + text.mid(1));
        this->ui_.textEdit->moveCursor(QTextCursor::EndOfBlock);
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

    bool hasReply = false;
    if (this->enableInlineReplying_)
    {
        if (this->replyThread_ != nullptr)
        {
            // Check if the input still starts with @username. If not, don't reply.
            //
            // We need to verify that
            // 1. the @username prefix exists and
            // 2. if a character exists after the @username, it is a space
            QString replyPrefix = "@" + this->replyThread_->root()->displayName;
            if (!text.startsWith(replyPrefix) ||
                (text.length() > replyPrefix.length() &&
                 text.at(replyPrefix.length()) != ' '))
            {
                this->replyThread_ = nullptr;
            }
        }

        // Show/hide reply label if inline replies are possible
        hasReply = this->replyThread_ != nullptr;
    }

    this->ui_.replyWrapper->setVisible(hasReply);
    this->ui_.replyLabel->setVisible(hasReply);
    this->ui_.cancelReplyButton->setVisible(hasReply);
}

void SplitInput::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);

    int s;
    QColor borderColor;

    if (this->theme->isLightTheme())
    {
        s = int(3 * this->scale());
        borderColor = QColor("#ccc");
    }
    else
    {
        s = int(1 * this->scale());
        borderColor = QColor("#333");
    }

    QMargins removeMargins(s - 1, s - 1, s, s);
    QRect baseRect = this->rect();

    // completeAreaRect includes the reply label
    QRect completeAreaRect = baseRect.marginsRemoved(removeMargins);
    painter.fillRect(completeAreaRect, this->theme->splits.input.background);
    painter.setPen(borderColor);
    painter.drawRect(completeAreaRect);

    if (this->enableInlineReplying_ && this->replyThread_ != nullptr)
    {
        // Move top of rect down to not include reply label
        baseRect.setTop(baseRect.top() + this->ui_.replyWrapper->height());

        QRect onlyInputRect = baseRect.marginsRemoved(removeMargins);
        painter.setPen(borderColor);
        painter.drawRect(onlyInputRect);
    }
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

void SplitInput::giveFocus(Qt::FocusReason reason)
{
    this->ui_.textEdit->setFocus(reason);
}

void SplitInput::setReply(std::shared_ptr<MessageThread> reply,
                          bool showReplyingLabel)
{
    this->replyThread_ = std::move(reply);

    if (this->enableInlineReplying_)
    {
        // Only enable reply label if inline replying
        this->ui_.textEdit->setPlainText(
            "@" + this->replyThread_->root()->displayName + " ");
        this->ui_.textEdit->moveCursor(QTextCursor::EndOfBlock);
        this->ui_.replyLabel->setText("Replying to @" +
                                      this->replyThread_->root()->displayName);
    }
}

void SplitInput::setPlaceholderText(const QString &text)
{
    this->ui_.textEdit->setPlaceholderText(text);
}

void SplitInput::clearInput()
{
    this->currMsg_ = "";
    this->ui_.textEdit->setText("");
    this->ui_.textEdit->moveCursor(QTextCursor::Start);
    if (this->enableInlineReplying_)
    {
        this->replyThread_ = nullptr;
    }
}

}  // namespace chatterino
