#include "widgets/helper/splitinput.hpp"
#include "singletons/commandmanager.hpp"
#include "singletons/completionmanager.hpp"
#include "singletons/ircmanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
#include "util/layoutcreator.hpp"
#include "widgets/notebook.hpp"
#include "widgets/split.hpp"
#include "widgets/splitcontainer.hpp"

#include <QCompleter>
#include <QPainter>

namespace chatterino {
namespace widgets {

SplitInput::SplitInput(Split *_chatWidget)
    : BaseWidget(_chatWidget)
    , chatWidget(_chatWidget)
{
    this->initLayout();

    // auto completion
    auto completer = new QCompleter(
        singletons::CompletionManager::getInstance().createModel(this->chatWidget->channelName));

    this->ui.textEdit->setCompleter(completer);

    // misc
    this->installKeyPressedEvent();
    this->themeRefreshEvent();
    this->scaleChangedEvent(this->getScale());
}

void SplitInput::initLayout()
{
    auto &fontManager = singletons::FontManager::getInstance();
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
        fontManager.getFont(singletons::FontManager::Type::Medium, this->getScale()));

    this->managedConnections.emplace_back(fontManager.fontChanged.connect([this, &fontManager]() {
        this->ui.textEdit->setFont(
            fontManager.getFont(singletons::FontManager::Type::Medium, this->getScale()));
    }));

    // open emote popup
    QObject::connect(this->ui.emoteButton, &RippleEffectLabel::clicked, [this] {
        if (!this->emotePopup) {
            this->emotePopup = std::make_unique<EmotePopup>(this->themeManager);
            this->emotePopup->linkClicked.connect([this](const messages::Link &link) {
                if (link.getType() == messages::Link::InsertText) {
                    this->insertText(link.getValue());
                }
            });
        }

        this->emotePopup->resize((int)(300 * this->emotePopup->getScale()),
                                 (int)(500 * this->emotePopup->getScale()));
        this->emotePopup->loadChannel(this->chatWidget->getChannel());
        this->emotePopup->show();
    });

    // clear channelview selection when selecting in the input
    QObject::connect(this->ui.textEdit, &QTextEdit::copyAvailable, [this](bool available) {
        if (available) {
            this->chatWidget->view.clearSelection();
        }
    });

    // textEditLength visibility
    singletons::SettingManager::getInstance().showMessageLength.connect(
        [this](const bool &value, auto) { this->ui.textEditLength->setHidden(!value); },
        this->managedConnections);
}

void SplitInput::scaleChangedEvent(float scale)
{
    // update the icon size of the emote button
    QString text = "<img src=':/images/emote.svg' width='xD' height='xD' />";
    text.replace("xD", QString::number((int)12 * scale));

    this->ui.emoteButton->getLabel().setText(text);
    this->ui.emoteButton->setFixedHeight((int)18 * scale);

    // set maximum height
    this->setMaximumHeight((int)(150 * this->getScale()));

    this->themeRefreshEvent();
}

void SplitInput::themeRefreshEvent()
{
    QPalette palette;

    palette.setColor(QPalette::Foreground, this->themeManager.splits.input.text);

    this->ui.textEditLength->setPalette(palette);

    this->ui.textEdit->setStyleSheet(this->themeManager.splits.input.styleSheet);

    this->ui.hbox->setMargin((this->themeManager.isLightTheme() ? 4 : 2) * this->getScale());
}

void SplitInput::installKeyPressedEvent()
{
    this->ui.textEdit->keyPressed.connect([this](QKeyEvent *event) {
        if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
            auto c = this->chatWidget->getChannel();
            if (c == nullptr) {
                return;
            }
            QString message = ui.textEdit->toPlainText();

            QString sendMessage =
                singletons::CommandManager::getInstance().execCommand(message, c, false);
            sendMessage = sendMessage.replace('\n', ' ');

            c->sendMessage(sendMessage);
            this->prevMsg.append(message);

            event->accept();
            if (!(event->modifiers() == Qt::ControlModifier)) {
                this->ui.textEdit->setText(QString());
                this->prevIndex = 0;
            } else if (this->ui.textEdit->toPlainText() ==
                       this->prevMsg.at(this->prevMsg.size() - 1)) {
                this->prevMsg.removeLast();
            }
            this->prevIndex = this->prevMsg.size();
        } else if (event->key() == Qt::Key_Up) {
            if (event->modifiers() == Qt::AltModifier) {
                SplitContainer *page =
                    static_cast<SplitContainer *>(this->chatWidget->parentWidget());

                int reqX = page->currentX;
                int reqY = page->lastRequestedY[reqX] - 1;

                qDebug() << "Alt+Down to" << reqX << "/" << reqY;

                page->requestFocus(reqX, reqY);
            } else {
                if (this->prevMsg.size() && this->prevIndex) {
                    this->prevIndex--;
                    this->ui.textEdit->setText(this->prevMsg.at(this->prevIndex));
                }
            }
        } else if (event->key() == Qt::Key_Down) {
            if (event->modifiers() == Qt::AltModifier) {
                SplitContainer *page =
                    static_cast<SplitContainer *>(this->chatWidget->parentWidget());

                int reqX = page->currentX;
                int reqY = page->lastRequestedY[reqX] + 1;

                qDebug() << "Alt+Down to" << reqX << "/" << reqY;

                page->requestFocus(reqX, reqY);
            } else {
                if (this->prevIndex != (this->prevMsg.size() - 1) &&
                    this->prevIndex != this->prevMsg.size()) {
                    this->prevIndex++;
                    this->ui.textEdit->setText(this->prevMsg.at(this->prevIndex));
                } else {
                    this->prevIndex = this->prevMsg.size();
                    this->ui.textEdit->setText(QString());
                }
            }
        } else if (event->key() == Qt::Key_Left) {
            if (event->modifiers() == Qt::AltModifier) {
                SplitContainer *page =
                    static_cast<SplitContainer *>(this->chatWidget->parentWidget());

                int reqX = page->currentX - 1;
                int reqY = page->lastRequestedY[reqX];

                qDebug() << "Alt+Left to" << reqX << "/" << reqY;

                page->requestFocus(reqX, reqY);
            }
        } else if (event->key() == Qt::Key_Right) {
            if (event->modifiers() == Qt::AltModifier) {
                SplitContainer *page =
                    static_cast<SplitContainer *>(this->chatWidget->parentWidget());

                int reqX = page->currentX + 1;
                int reqY = page->lastRequestedY[reqX];

                qDebug() << "Alt+Right to" << reqX << "/" << reqY;

                page->requestFocus(reqX, reqY);
            }
        } else if (event->key() == Qt::Key_Tab) {
            if (event->modifiers() == Qt::ControlModifier) {
                SplitContainer *page =
                    static_cast<SplitContainer *>(this->chatWidget->parentWidget());

                Notebook *notebook = static_cast<Notebook *>(page->parentWidget());

                notebook->nextTab();
            }
        } else if (event->key() == Qt::Key_Backtab) {
            if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
                SplitContainer *page =
                    static_cast<SplitContainer *>(this->chatWidget->parentWidget());

                Notebook *notebook = static_cast<Notebook *>(page->parentWidget());

                notebook->previousTab();
            }
        } else if (event->key() == Qt::Key_C && event->modifiers() == Qt::ControlModifier) {
            if (this->chatWidget->view.hasSelection()) {
                this->chatWidget->doCopy();
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
    // set textLengthLabel value
    QString text = this->ui.textEdit->toPlainText();

    this->textChanged.invoke(text);

    text = text.trimmed();
    static QRegularExpression spaceRegex("\\s\\s+");
    text = text.replace(spaceRegex, " ");

    text = singletons::CommandManager::getInstance().execCommand(
        text, this->chatWidget->getChannel(), true);

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

    painter.fillRect(this->rect(), this->themeManager.splits.input.background);

    QPen pen(this->themeManager.splits.input.border);
    if (this->themeManager.isLightTheme()) {
        pen.setWidth((int)(6 * this->getScale()));
    }
    painter.setPen(pen);
    painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
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
    this->chatWidget->giveFocus(Qt::MouseFocusReason);
}

}  // namespace widgets
}  // namespace chatterino
