#include "widgets/helper/splitinput.hpp"

#include "application.hpp"
#include "controllers/commands/commandcontroller.hpp"
#include "providers/twitch/twitchchannel.hpp"
#include "providers/twitch/twitchserver.hpp"
#include "singletons/ircmanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
#include "util/layoutcreator.hpp"
#include "util/urlfetch.hpp"
#include "widgets/notebook.hpp"
#include "widgets/split.hpp"
#include "widgets/splitcontainer.hpp"

#include <QCompleter>
#include <QPainter>

namespace chatterino {
namespace widgets {

SplitInput::SplitInput(Split *_chatWidget)
    : BaseWidget(_chatWidget)
    , split_(_chatWidget)
{
    this->initLayout();

    auto completer = new QCompleter(&this->split_->getChannel().get()->completionModel);
    this->ui_.textEdit->setCompleter(completer);

    this->split_->channelChanged.connect([this] {
        auto completer = new QCompleter(&this->split_->getChannel()->completionModel);
        this->ui_.textEdit->setCompleter(completer);
    });

    // misc
    this->installKeyPressedEvent();
    this->scaleChangedEvent(this->getScale());
}

void SplitInput::initLayout()
{
    auto app = getApp();
    util::LayoutCreator<SplitInput> layoutCreator(this);

    auto layout =
        layoutCreator.setLayoutType<QHBoxLayout>().withoutMargin().assign(&this->ui_.hbox);

    // input
    auto textEdit = layout.emplace<ResizingTextEdit>().assign(&this->ui_.textEdit);
    connect(textEdit.getElement(), &ResizingTextEdit::textChanged, this,
            &SplitInput::editTextChanged);

    // right box
    auto box = layout.emplace<QVBoxLayout>().withoutMargin();
    box->setSpacing(0);
    {
        auto textEditLength = box.emplace<QLabel>().assign(&this->ui_.textEditLength);
        textEditLength->setAlignment(Qt::AlignRight);

        box->addStretch(1);
        box.emplace<RippleEffectLabel>().assign(&this->ui_.emoteButton);
    }

    this->ui_.emoteButton->getLabel().setTextFormat(Qt::RichText);

    // ---- misc

    // set edit font
    this->ui_.textEdit->setFont(
        app->fonts->getFont(singletons::FontManager::Type::ChatMedium, this->getScale()));

    this->managedConnections_.emplace_back(app->fonts->fontChanged.connect([=]() {
        this->ui_.textEdit->setFont(
            app->fonts->getFont(singletons::FontManager::Type::ChatMedium, this->getScale()));
    }));

    // open emote popup
    QObject::connect(this->ui_.emoteButton, &RippleEffectLabel::clicked, [this] {
        if (!this->emotePopup_) {
            this->emotePopup_ = std::make_unique<EmotePopup>();
            this->emotePopup_->linkClicked.connect([this](const messages::Link &link) {
                if (link.type == messages::Link::InsertText) {
                    this->insertText(link.value + " ");
                }
            });
        }

        this->emotePopup_->resize(int(300 * this->emotePopup_->getScale()),
                                  int(500 * this->emotePopup_->getScale()));
        this->emotePopup_->loadChannel(this->split_->getChannel());
        this->emotePopup_->show();
    });

    // clear channelview selection when selecting in the input
    QObject::connect(this->ui_.textEdit, &QTextEdit::copyAvailable, [this](bool available) {
        if (available) {
            this->split_->view.clearSelection();
        }
    });

    // textEditLength visibility
    app->settings->showMessageLength.connect(
        [this](const bool &value, auto) { this->ui_.textEditLength->setHidden(!value); },
        this->managedConnections_);
}

void SplitInput::scaleChangedEvent(float scale)
{
    // update the icon size of the emote button
    QString text = "<img src=':/images/emote.svg' width='xD' height='xD' />";
    text.replace("xD", QString::number(int(12 * scale)));

    this->ui_.emoteButton->getLabel().setText(text);
    this->ui_.emoteButton->setFixedHeight(int(18 * scale));

    // set maximum height
    this->setMaximumHeight(int(150 * this->getScale()));
}

void SplitInput::themeRefreshEvent()
{
    QPalette palette;

    palette.setColor(QPalette::Foreground, this->themeManager->splits.input.text);

    this->ui_.textEditLength->setPalette(palette);

    this->ui_.textEdit->setStyleSheet(this->themeManager->splits.input.styleSheet);

    this->ui_.hbox->setMargin(int((this->themeManager->isLightTheme() ? 4 : 2) * this->getScale()));
}

void SplitInput::installKeyPressedEvent()
{
    auto app = getApp();

    this->ui_.textEdit->keyPressed.connect([this, app](QKeyEvent *event) {
        if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
            auto c = this->split_->getChannel();
            if (c == nullptr) {
                return;
            }

            QString message = ui_.textEdit->toPlainText();

            QString sendMessage = app->commands->execCommand(message, c, false);
            sendMessage = sendMessage.replace('\n', ' ');

            c->sendMessage(sendMessage);
            // don't add duplicate messages to message history
            if (this->prevMsg_.isEmpty() || !this->prevMsg_.endsWith(message))
                this->prevMsg_.append(message);

            event->accept();
            if (!(event->modifiers() == Qt::ControlModifier)) {
                this->currMsg_ = QString();
                this->ui_.textEdit->setText(QString());
                this->prevIndex_ = 0;
            } else if (this->ui_.textEdit->toPlainText() ==
                       this->prevMsg_.at(this->prevMsg_.size() - 1)) {
                this->prevMsg_.removeLast();
            }
            this->prevIndex_ = this->prevMsg_.size();
        } else if (event->key() == Qt::Key_Up) {
            if ((event->modifiers() & Qt::ShiftModifier) != 0) {
                return;
            }
            if (event->modifiers() == Qt::AltModifier) {
                SplitContainer *page = this->split_->getContainer();

                if (page != nullptr) {
                    page->selectNextSplit(SplitContainer::Above);
                }
            } else {
                if (this->prevMsg_.size() && this->prevIndex_) {
                    if (this->prevIndex_ == (this->prevMsg_.size())) {
                        this->currMsg_ = ui_.textEdit->toPlainText();
                    }

                    this->prevIndex_--;
                    this->ui_.textEdit->setText(this->prevMsg_.at(this->prevIndex_));

                    QTextCursor cursor = this->ui_.textEdit->textCursor();
                    cursor.movePosition(QTextCursor::End);
                    this->ui_.textEdit->setTextCursor(cursor);
                }
            }
        } else if (event->key() == Qt::Key_Down) {
            if ((event->modifiers() & Qt::ShiftModifier) != 0) {
                return;
            }
            if (event->modifiers() == Qt::AltModifier) {
                SplitContainer *page = this->split_->getContainer();

                if (page != nullptr) {
                    page->selectNextSplit(SplitContainer::Below);
                }
            } else {
                if (this->prevIndex_ != (this->prevMsg_.size() - 1) &&
                    this->prevIndex_ != this->prevMsg_.size()) {
                    this->prevIndex_++;
                    this->ui_.textEdit->setText(this->prevMsg_.at(this->prevIndex_));
                } else {
                    this->prevIndex_ = this->prevMsg_.size();
                    this->ui_.textEdit->setText(this->currMsg_);
                }

                QTextCursor cursor = this->ui_.textEdit->textCursor();
                cursor.movePosition(QTextCursor::End);
                this->ui_.textEdit->setTextCursor(cursor);
            }
        } else if (event->key() == Qt::Key_Left) {
            if (event->modifiers() == Qt::AltModifier) {
                SplitContainer *page = this->split_->getContainer();

                if (page != nullptr) {
                    page->selectNextSplit(SplitContainer::Left);
                }
            }
        } else if (event->key() == Qt::Key_Right) {
            if (event->modifiers() == Qt::AltModifier) {
                SplitContainer *page = this->split_->getContainer();

                if (page != nullptr) {
                    page->selectNextSplit(SplitContainer::Right);
                }
            }
        } else if (event->key() == Qt::Key_Tab) {
            if (event->modifiers() == Qt::ControlModifier) {
                SplitContainer *page = static_cast<SplitContainer *>(this->split_->parentWidget());

                Notebook *notebook = static_cast<Notebook *>(page->parentWidget());

                notebook->selectNextTab();
            }
        } else if (event->key() == Qt::Key_Backtab) {
            if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
                SplitContainer *page = static_cast<SplitContainer *>(this->split_->parentWidget());

                Notebook *notebook = static_cast<Notebook *>(page->parentWidget());

                notebook->selectPreviousTab();
            }
        } else if (event->key() == Qt::Key_C && event->modifiers() == Qt::ControlModifier) {
            if (this->split_->view.hasSelection()) {
                this->split_->doCopy();
                event->accept();
            }
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
        if (!lastUser.isEmpty()) {
            this->ui_.textEdit->setPlainText("/w " + lastUser + text.mid(2));
            this->ui_.textEdit->moveCursor(QTextCursor::EndOfBlock);
        }
    } else {
        this->textChanged.invoke(text);

        text = text.trimmed();
        static QRegularExpression spaceRegex("\\s\\s+");
        text = text.replace(spaceRegex, " ");

        text = app->commands->execCommand(text, this->split_->getChannel(), true);
    }

    QString labelText;

    if (text.length() == 0) {
        labelText = "";
    } else {
        labelText = QString::number(text.length());
    }

    this->ui_.textEditLength->setText(labelText);
}

void SplitInput::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (this->themeManager->isLightTheme()) {
        int s = int(3 * this->getScale());
        QRect rect = this->rect().marginsRemoved(QMargins(s, s, s, s));

        painter.fillRect(rect, this->themeManager->splits.input.background);

        painter.setPen(QColor("#ccc"));
        painter.drawRect(rect);
    } else {
        int s = int(1 * this->getScale());
        QRect rect = this->rect().marginsRemoved(QMargins(s, s, s, s));

        painter.fillRect(rect, this->themeManager->splits.input.background);

        painter.setPen(QColor("#333"));
        painter.drawRect(rect);
    }

    //    int offset = 2;
    //    painter.fillRect(offset, this->height() - offset, this->width() - 2 * offset, 1,
    //                     getApp()->themes->splits.input.focusedLine);
}

void SplitInput::resizeEvent(QResizeEvent *)
{
    if (this->height() == this->maximumHeight()) {
        this->ui_.textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    } else {
        this->ui_.textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
}

void SplitInput::mousePressEvent(QMouseEvent *)
{
    this->split_->giveFocus(Qt::MouseFocusReason);
}

}  // namespace widgets
}  // namespace chatterino
