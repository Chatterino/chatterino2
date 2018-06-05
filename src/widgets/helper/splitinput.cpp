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
    , split(_chatWidget)
{
    this->initLayout();

    auto completer = new QCompleter(&this->split->getChannel().get()->completionModel);
    this->ui.textEdit->setCompleter(completer);

    this->split->channelChanged.connect([this] {
        auto completer = new QCompleter(&this->split->getChannel()->completionModel);
        this->ui.textEdit->setCompleter(completer);
    });

    // misc
    this->installKeyPressedEvent();
    this->scaleChangedEvent(this->getScale());
}

void SplitInput::initLayout()
{
    auto app = getApp();
    util::LayoutCreator<SplitInput> layoutCreator(this);

    auto layout = layoutCreator.setLayoutType<QHBoxLayout>().withoutMargin().assign(&this->ui.hbox);

    // input
    auto textEdit = layout.emplace<ResizingTextEdit>().assign(&this->ui.textEdit);
    connect(textEdit.getElement(), &ResizingTextEdit::textChanged, this,
            &SplitInput::editTextChanged);

    // right box
    auto box = layout.emplace<QVBoxLayout>().withoutMargin();
    box->setSpacing(0);
    {
        auto textEditLength = box.emplace<QLabel>().assign(&this->ui.textEditLength);
        textEditLength->setAlignment(Qt::AlignRight);

        box->addStretch(1);
        box.emplace<RippleEffectLabel>().assign(&this->ui.emoteButton);
    }

    this->ui.emoteButton->getLabel().setTextFormat(Qt::RichText);

    // ---- misc

    // set edit font
    this->ui.textEdit->setFont(
        app->fonts->getFont(singletons::FontManager::Type::ChatMedium, this->getScale()));

    this->managedConnections.emplace_back(app->fonts->fontChanged.connect([=]() {
        this->ui.textEdit->setFont(
            app->fonts->getFont(singletons::FontManager::Type::ChatMedium, this->getScale()));
    }));

    // open emote popup
    QObject::connect(this->ui.emoteButton, &RippleEffectLabel::clicked, [this] {
        if (!this->emotePopup) {
            this->emotePopup = std::make_unique<EmotePopup>();
            this->emotePopup->linkClicked.connect([this](const messages::Link &link) {
                if (link.type == messages::Link::InsertText) {
                    this->insertText(link.value + " ");
                }
            });
        }

        this->emotePopup->resize(int(300 * this->emotePopup->getScale()),
                                 int(500 * this->emotePopup->getScale()));
        this->emotePopup->loadChannel(this->split->getChannel());
        this->emotePopup->show();
    });

    // clear channelview selection when selecting in the input
    QObject::connect(this->ui.textEdit, &QTextEdit::copyAvailable, [this](bool available) {
        if (available) {
            this->split->view.clearSelection();
        }
    });

    // textEditLength visibility
    app->settings->showMessageLength.connect(
        [this](const bool &value, auto) { this->ui.textEditLength->setHidden(!value); },
        this->managedConnections);
}

void SplitInput::scaleChangedEvent(float scale)
{
    // update the icon size of the emote button
    QString text = "<img src=':/images/emote.svg' width='xD' height='xD' />";
    text.replace("xD", QString::number(int(12 * scale)));

    this->ui.emoteButton->getLabel().setText(text);
    this->ui.emoteButton->setFixedHeight(int(18 * scale));

    // set maximum height
    this->setMaximumHeight(int(150 * this->getScale()));
}

void SplitInput::themeRefreshEvent()
{
    QPalette palette;

    palette.setColor(QPalette::Foreground, this->themeManager->splits.input.text);

    this->ui.textEditLength->setPalette(palette);

    this->ui.textEdit->setStyleSheet(this->themeManager->splits.input.styleSheet);

    this->ui.hbox->setMargin(int((this->themeManager->isLightTheme() ? 4 : 2) * this->getScale()));
}

void SplitInput::installKeyPressedEvent()
{
    auto app = getApp();

    this->ui.textEdit->keyPressed.connect([this, app](QKeyEvent *event) {
        if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
            auto c = this->split->getChannel();
            if (c == nullptr) {
                return;
            }
            QString message = ui.textEdit->toPlainText();

            QString sendMessage = app->commands->execCommand(message, c, false);
            sendMessage = sendMessage.replace('\n', ' ');

            c->sendMessage(sendMessage);
            // don't add duplicate messages to message history
            if (this->prevMsg.isEmpty() || !this->prevMsg.endsWith(message))
                this->prevMsg.append(message);

            event->accept();
            if (!(event->modifiers() == Qt::ControlModifier)) {
                this->currMsg = QString();
                this->ui.textEdit->setText(QString());
                this->prevIndex = 0;
            } else if (this->ui.textEdit->toPlainText() ==
                       this->prevMsg.at(this->prevMsg.size() - 1)) {
                this->prevMsg.removeLast();
            }
            this->prevIndex = this->prevMsg.size();
        } else if (event->key() == Qt::Key_Up) {
            if ((event->modifiers() & Qt::ShiftModifier) != 0) {
                return;
            }
            if (event->modifiers() == Qt::AltModifier) {
                SplitContainer *page = this->split->getContainer();

                if (page != nullptr) {
                    page->selectNextSplit(SplitContainer::Above);
                }
            } else {
                if (this->prevMsg.size() && this->prevIndex) {
                    if (this->prevIndex == (this->prevMsg.size())) {
                        this->currMsg = ui.textEdit->toPlainText();
                    }

                    this->prevIndex--;
                    this->ui.textEdit->setText(this->prevMsg.at(this->prevIndex));

                    QTextCursor cursor = this->ui.textEdit->textCursor();
                    cursor.movePosition(QTextCursor::End);
                    this->ui.textEdit->setTextCursor(cursor);
                }
            }
        } else if (event->key() == Qt::Key_Down) {
            if ((event->modifiers() & Qt::ShiftModifier) != 0) {
                return;
            }
            if (event->modifiers() == Qt::AltModifier) {
                SplitContainer *page = this->split->getContainer();

                if (page != nullptr) {
                    page->selectNextSplit(SplitContainer::Below);
                }
            } else {
                if (this->prevIndex != (this->prevMsg.size() - 1) &&
                    this->prevIndex != this->prevMsg.size()) {
                    this->prevIndex++;
                    this->ui.textEdit->setText(this->prevMsg.at(this->prevIndex));
                } else {
                    this->prevIndex = this->prevMsg.size();
                    this->ui.textEdit->setText(this->currMsg);
                }

                QTextCursor cursor = this->ui.textEdit->textCursor();
                cursor.movePosition(QTextCursor::End);
                this->ui.textEdit->setTextCursor(cursor);
            }
        } else if (event->key() == Qt::Key_Left) {
            if (event->modifiers() == Qt::AltModifier) {
                SplitContainer *page = this->split->getContainer();

                if (page != nullptr) {
                    page->selectNextSplit(SplitContainer::Left);
                }
            }
        } else if (event->key() == Qt::Key_Right) {
            if (event->modifiers() == Qt::AltModifier) {
                SplitContainer *page = this->split->getContainer();

                if (page != nullptr) {
                    page->selectNextSplit(SplitContainer::Right);
                }
            }
        } else if (event->key() == Qt::Key_Tab) {
            if (event->modifiers() == Qt::ControlModifier) {
                SplitContainer *page = static_cast<SplitContainer *>(this->split->parentWidget());

                Notebook *notebook = static_cast<Notebook *>(page->parentWidget());

                notebook->selectNextTab();
            }
        } else if (event->key() == Qt::Key_Backtab) {
            if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
                SplitContainer *page = static_cast<SplitContainer *>(this->split->parentWidget());

                Notebook *notebook = static_cast<Notebook *>(page->parentWidget());

                notebook->selectPreviousTab();
            }
        } else if (event->key() == Qt::Key_C && event->modifiers() == Qt::ControlModifier) {
            if (this->split->view.hasSelection()) {
                this->split->doCopy();
                event->accept();
            }
        }
    });
}

void SplitInput::clearSelection()
{
    QTextCursor c = this->ui.textEdit->textCursor();

    c.setPosition(c.position());
    c.setPosition(c.position(), QTextCursor::KeepAnchor);

    this->ui.textEdit->setTextCursor(c);
}

QString SplitInput::getInputText() const
{
    return this->ui.textEdit->toPlainText();
}

void SplitInput::insertText(const QString &text)
{
    this->ui.textEdit->insertPlainText(text);
}

void SplitInput::editTextChanged()
{
    auto app = getApp();

    // set textLengthLabel value
    QString text = this->ui.textEdit->toPlainText();

    if (text.startsWith("/r ") && this->split->getChannel()->isTwitchChannel())  //
    {
        QString lastUser = app->twitch.server->lastUserThatWhisperedMe.get();
        if (!lastUser.isEmpty()) {
            this->ui.textEdit->setPlainText("/w " + lastUser + text.mid(2));
            this->ui.textEdit->moveCursor(QTextCursor::EndOfBlock);
        }
    } else {
        this->textChanged.invoke(text);

        text = text.trimmed();
        static QRegularExpression spaceRegex("\\s\\s+");
        text = text.replace(spaceRegex, " ");

        text = app->commands->execCommand(text, this->split->getChannel(), true);
    }

    QString labelText;

    if (text.length() == 0) {
        labelText = "";
    } else {
        labelText = QString::number(text.length());
    }

    this->ui.textEditLength->setText(labelText);
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
}

void SplitInput::resizeEvent(QResizeEvent *)
{
    if (this->height() == this->maximumHeight()) {
        this->ui.textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    } else {
        this->ui.textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
}

void SplitInput::mousePressEvent(QMouseEvent *)
{
    this->split->giveFocus(Qt::MouseFocusReason);
}

}  // namespace widgets
}  // namespace chatterino
