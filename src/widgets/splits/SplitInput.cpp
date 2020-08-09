#include "widgets/splits/SplitInput.hpp"

#include "Application.hpp"
#include "controllers/commands/CommandController.hpp"
#include "messages/Link.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Scrollbar.hpp"
#include "widgets/dialogs/EmotePopup.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/EffectLabel.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"
#include "widgets/splits/SplitInput.hpp"

#include <QCompleter>
#include <QPainter>

namespace chatterino {

SplitInput::SplitInput(Split *_chatWidget)
    : BaseWidget(_chatWidget)
    , split_(_chatWidget)
{
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
    this->scaleChangedEvent(this->scale());
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

    this->managedConnections_.push_back(app->fonts->fontChanged.connect([=]() {
        this->ui_.textEdit->setFont(
            app->fonts->getFont(FontStyle::ChatMedium, this->scale()));
    }));

    // open emote popup
    QObject::connect(this->ui_.emoteButton, &EffectLabel::leftClicked,
                     [=] { this->openEmotePopup(); });

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
    QPalette palette;

    palette.setColor(QPalette::Foreground, this->theme->splits.input.text);

    this->updateEmoteButton();
    this->ui_.textEditLength->setPalette(palette);

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

void SplitInput::installKeyPressedEvent()
{
    auto app = getApp();

    this->ui_.textEdit->keyPressed.connect([this, app](QKeyEvent *event) {
        if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        {
            auto c = this->split_->getChannel();
            if (c == nullptr)
                return;

            QString message = ui_.textEdit->toPlainText();

            message = message.replace('\n', ' ');
            QString sendMessage = app->commands->execCommand(message, c, false);

            c->sendMessage(sendMessage);
            // don't add duplicate messages and empty message to message history
            if ((this->prevMsg_.isEmpty() ||
                 !this->prevMsg_.endsWith(message)) &&
                !message.trimmed().isEmpty())
            {
                this->prevMsg_.append(message);
            }

            event->accept();
            if (!(event->modifiers() & Qt::ControlModifier))
            {
                this->currMsg_ = QString();
                this->ui_.textEdit->setPlainText(QString());
            }
            this->prevIndex_ = this->prevMsg_.size();
        }
        else if (event->key() == Qt::Key_Up)
        {
            if ((event->modifiers() & Qt::ShiftModifier) != 0)
            {
                return;
            }
            if (event->modifiers() == Qt::AltModifier)
            {
                SplitContainer *page = this->split_->getContainer();

                if (page != nullptr)
                {
                    page->selectNextSplit(SplitContainer::Above);
                }
            }
            else
            {
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

                    // Don't let the keyboard event propagate further, we've
                    // handled it
                    event->accept();
                }
            }
        }
        else if (event->key() == Qt::Key_Home)
        {
            QTextCursor cursor = this->ui_.textEdit->textCursor();
            cursor.movePosition(
                QTextCursor::Start,
                event->modifiers() & Qt::KeyboardModifier::ShiftModifier
                    ? QTextCursor::MoveMode::KeepAnchor
                    : QTextCursor::MoveMode::MoveAnchor);
            this->ui_.textEdit->setTextCursor(cursor);

            event->accept();
        }
        else if (event->key() == Qt::Key_End)
        {
            if (event->modifiers() == Qt::ControlModifier)
            {
                this->split_->getChannelView().getScrollBar().scrollToBottom(
                    getSettings()->enableSmoothScrollingNewMessages.getValue());
            }
            else
            {
                QTextCursor cursor = this->ui_.textEdit->textCursor();
                cursor.movePosition(
                    QTextCursor::End,
                    event->modifiers() & Qt::KeyboardModifier::ShiftModifier
                        ? QTextCursor::MoveMode::KeepAnchor
                        : QTextCursor::MoveMode::MoveAnchor);
                this->ui_.textEdit->setTextCursor(cursor);
            }
            event->accept();
        }
        else if (event->key() == Qt::Key_H &&
                 event->modifiers() == Qt::AltModifier)
        {
            // h: vim binding for left
            SplitContainer *page = this->split_->getContainer();
            event->accept();

            if (page != nullptr)
            {
                page->selectNextSplit(SplitContainer::Left);
            }
        }
        else if (event->key() == Qt::Key_J &&
                 event->modifiers() == Qt::AltModifier)
        {
            // j: vim binding for down
            SplitContainer *page = this->split_->getContainer();
            event->accept();

            if (page != nullptr)
            {
                page->selectNextSplit(SplitContainer::Below);
            }
        }
        else if (event->key() == Qt::Key_K &&
                 event->modifiers() == Qt::AltModifier)
        {
            // k: vim binding for up
            SplitContainer *page = this->split_->getContainer();
            event->accept();

            if (page != nullptr)
            {
                page->selectNextSplit(SplitContainer::Above);
            }
        }
        else if (event->key() == Qt::Key_L &&
                 event->modifiers() == Qt::AltModifier)
        {
            // l: vim binding for right
            SplitContainer *page = this->split_->getContainer();
            event->accept();

            if (page != nullptr)
            {
                page->selectNextSplit(SplitContainer::Right);
            }
        }
        else if (event->key() == Qt::Key_Down)
        {
            if ((event->modifiers() & Qt::ShiftModifier) != 0)
            {
                return;
            }
            if (event->modifiers() == Qt::AltModifier)
            {
                SplitContainer *page = this->split_->getContainer();

                if (page != nullptr)
                {
                    page->selectNextSplit(SplitContainer::Below);
                }
            }
            else
            {
                // If user did not write anything before then just do nothing.
                if (this->prevMsg_.isEmpty())
                {
                    return;
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
            }
        }
        else if (event->key() == Qt::Key_Left)
        {
            if (event->modifiers() == Qt::AltModifier)
            {
                SplitContainer *page = this->split_->getContainer();

                if (page != nullptr)
                {
                    page->selectNextSplit(SplitContainer::Left);
                }
            }
        }
        else if (event->key() == Qt::Key_Right)
        {
            if (event->modifiers() == Qt::AltModifier)
            {
                SplitContainer *page = this->split_->getContainer();

                if (page != nullptr)
                {
                    page->selectNextSplit(SplitContainer::Right);
                }
            }
        }
        else if (event->key() == Qt::Key_C &&
                 event->modifiers() == Qt::ControlModifier)
        {
            if (this->split_->view_->hasSelection())
            {
                this->split_->copyToClipboard();
                event->accept();
            }
        }
        else if (event->key() == Qt::Key_E &&
                 event->modifiers() == Qt::ControlModifier)
        {
            this->openEmotePopup();
        }
    });
}

void SplitInput::clearSelection()
{
    QTextCursor c = this->ui_.textEdit->textCursor();

    c.setPosition(c.position());
    c.setPosition(c.position(), QTextCursor::KeepAnchor);

    this->ui_.textEdit->setTextCursor(c);
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
        this->split_->getChannel()->isTwitchChannel())  //
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
        static QRegularExpression spaceRegex("\\s\\s+");
        text = text.replace(spaceRegex, " ");

        text =
            app->commands->execCommand(text, this->split_->getChannel(), true);
    }

    QString labelText;

    if (text.length() > 0 && getSettings()->showMessageLength)
    {
        labelText = QString::number(text.length());
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
