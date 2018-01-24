#include "widgets/helper/splitinput.hpp"
#include "singletons/commandmanager.hpp"
#include "singletons/completionmanager.hpp"
#include "singletons/ircmanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
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
    , emotesLabel(this)
{
    this->setLayout(&this->hbox);

    this->hbox.setMargin(4);

    this->hbox.addLayout(&this->editContainer);
    this->hbox.addLayout(&this->vbox);

    auto &fontManager = singletons::FontManager::getInstance();

    this->textInput.setFont(
        fontManager.getFont(singletons::FontManager::Type::Medium, this->getDpiMultiplier()));
    this->managedConnections.emplace_back(fontManager.fontChanged.connect([this, &fontManager]() {
        this->textInput.setFont(
            fontManager.getFont(singletons::FontManager::Type::Medium, this->getDpiMultiplier()));
    }));

    this->editContainer.addWidget(&this->textInput);
    this->editContainer.setMargin(2);

    this->emotesLabel.setMinimumHeight(24);

    this->vbox.addWidget(&this->textLengthLabel);
    this->vbox.addStretch(1);
    this->vbox.addWidget(&this->emotesLabel);

    this->textLengthLabel.setText("");
    this->textLengthLabel.setAlignment(Qt::AlignRight);

    this->emotesLabel.getLabel().setTextFormat(Qt::RichText);
    this->emotesLabel.getLabel().setText("<img src=':/images/emote.svg' width='12' height='12' "
                                         "/>");

    connect(&this->emotesLabel, &RippleEffectLabel::clicked, [this] {
        if (!this->emotePopup) {
            this->emotePopup = std::make_unique<EmotePopup>(this->themeManager);
            this->emotePopup->linkClicked.connect([this](const messages::Link &link) {
                if (link.getType() == messages::Link::InsertText) {
                    this->insertText(link.getValue());
                }
            });
        }

        this->emotePopup->resize((int)(300 * this->emotePopup->getDpiMultiplier()),
                                 (int)(500 * this->emotePopup->getDpiMultiplier()));
        this->emotePopup->loadChannel(this->chatWidget->getChannel());
        this->emotePopup->show();
    });

    connect(&textInput, &ResizingTextEdit::textChanged, this, &SplitInput::editTextChanged);

    this->refreshTheme();
    textLengthLabel.setHidden(!singletons::SettingManager::getInstance().showMessageLength);

    auto completer = new QCompleter(
        singletons::CompletionManager::getInstance().createModel(this->chatWidget->channelName));

    this->textInput.setCompleter(completer);

    this->textInput.keyPressed.connect([this](QKeyEvent *event) {
        if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
            auto c = this->chatWidget->getChannel();
            if (c == nullptr) {
                return;
            }
            QString message = textInput.toPlainText();

            QString sendMessage =
                singletons::CommandManager::getInstance().execCommand(message, c, false);
            sendMessage = sendMessage.replace('\n', ' ');

            c->sendMessage(sendMessage);
            this->prevMsg.append(message);

            event->accept();
            if (!(event->modifiers() == Qt::ControlModifier)) {
                this->textInput.setText(QString());
                this->prevIndex = 0;
            } else if (this->textInput.toPlainText() ==
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
                    this->textInput.setText(this->prevMsg.at(this->prevIndex));
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
                    this->textInput.setText(this->prevMsg.at(this->prevIndex));
                } else {
                    this->prevIndex = this->prevMsg.size();
                    this->textInput.setText(QString());
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

    singletons::SettingManager::getInstance().showMessageLength.connect(
        [this](const bool &value, auto) { this->textLengthLabel.setHidden(!value); },
        this->managedConnections);

    QObject::connect(&this->textInput, &QTextEdit::copyAvailable, [this](bool available) {
        if (available) {
            this->chatWidget->view.clearSelection();
        }
    });
}

void SplitInput::clearSelection()
{
    QTextCursor c = this->textInput.textCursor();

    c.setPosition(c.position());
    c.setPosition(c.position(), QTextCursor::KeepAnchor);

    this->textInput.setTextCursor(c);
}

QString SplitInput::getInputText() const
{
    return this->textInput.toPlainText();
}

void SplitInput::insertText(const QString &text)
{
    this->textInput.insertPlainText(text);
}

void SplitInput::refreshTheme()
{
    QPalette palette;

    palette.setColor(QPalette::Foreground, this->themeManager.splits.input.text);

    this->textLengthLabel.setPalette(palette);

    this->textInput.setStyleSheet(this->themeManager.splits.input.styleSheet);

    this->hbox.setMargin((this->themeManager.isLightTheme() ? 4 : 2) * this->getDpiMultiplier());
}

void SplitInput::editTextChanged()
{
    QString text = this->textInput.toPlainText();

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

    this->textLengthLabel.setText(labelText);
}

void SplitInput::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(this->rect(), this->themeManager.splits.input.background);

    QPen pen(this->themeManager.splits.input.border);
    if (this->themeManager.isLightTheme()) {
        pen.setWidth((int)(6 * this->getDpiMultiplier()));
    }
    painter.setPen(pen);
    painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
}

void SplitInput::resizeEvent(QResizeEvent *)
{
    if (this->height() == this->maximumHeight()) {
        this->textInput.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    } else {
        this->textInput.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    this->setMaximumHeight((int)(150 * this->getDpiMultiplier()));

    this->refreshTheme();
}

void SplitInput::mousePressEvent(QMouseEvent *)
{
    this->chatWidget->giveFocus(Qt::MouseFocusReason);
}

}  // namespace widgets
}  // namespace chatterino
