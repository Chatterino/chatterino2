#include "widgets/chatwidgetinput.hpp"
#include "chatwidget.hpp"
#include "colorscheme.hpp"
#include "completionmanager.hpp"
#include "ircmanager.hpp"
#include "notebook.hpp"
#include "notebookpage.hpp"
#include "settingsmanager.hpp"

#include <QCompleter>
#include <QPainter>

namespace chatterino {
namespace widgets {

ChatWidgetInput::ChatWidgetInput(ChatWidget *_chatWidget, EmoteManager &emoteManager,
                                 WindowManager &windowManager)
    : BaseWidget(_chatWidget)
    , chatWidget(_chatWidget)
    , emoteManager(emoteManager)
    , windowManager(windowManager)
    , emotesLabel(this)
{
    this->setMaximumHeight(150);

    this->setLayout(&this->hbox);

    this->hbox.setMargin(0);

    this->hbox.addLayout(&this->editContainer);
    this->hbox.addLayout(&this->vbox);

    this->editContainer.addWidget(&this->textInput);
    this->editContainer.setMargin(4);

    this->emotesLabel.setMinimumHeight(24);

    this->vbox.addWidget(&this->textLengthLabel);
    this->vbox.addStretch(1);
    this->vbox.addWidget(&this->emotesLabel);

    this->textLengthLabel.setText("100");
    this->textLengthLabel.setAlignment(Qt::AlignRight);

    this->emotesLabel.getLabel().setTextFormat(Qt::RichText);
    this->emotesLabel.getLabel().setText(
        "<img src=':/images/Emoji_Color_1F60A_19.png' width='12' height='12' "
        "/>");

    connect(&this->emotesLabel, &RippleEffectLabel::clicked, [this] {
        if (this->emotePopup == nullptr) {
            this->emotePopup =
                new EmotePopup(this->colorScheme, this->emoteManager, this->windowManager);
        }

        this->emotePopup->resize(300, 500);
        this->emotePopup->loadChannel(this->chatWidget->getChannel());
        this->emotePopup->show();
    });

    connect(&textInput, &ResizingTextEdit::textChanged, this, &ChatWidgetInput::editTextChanged);

    this->refreshTheme();
    textLengthLabel.setHidden(!SettingsManager::getInstance().showMessageLength.get());

    auto completer = new QCompleter(
        this->chatWidget->completionManager.createModel(this->chatWidget->channelName));

    this->textInput.setCompleter(completer);

    this->textInput.keyPressed.connect([this](QKeyEvent *event) {
        if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
            auto c = this->chatWidget->getChannel();
            if (c == nullptr) {
                return;
            }
            QString message = textInput.toPlainText();

            c->sendMessage(message.replace('\n', ' '));
            prevMsg.append(message);

            event->accept();
            if (!(event->modifiers() == Qt::ControlModifier)) {
                textInput.setText(QString());
                prevIndex = 0;
            } else if (textInput.toPlainText() == prevMsg.at(prevMsg.size() - 1)) {
                prevMsg.removeLast();
            }
            prevIndex = prevMsg.size();
        } else if (event->key() == Qt::Key_Up) {
            if (event->modifiers() == Qt::AltModifier) {
                NotebookPage *page = static_cast<NotebookPage *>(this->chatWidget->parentWidget());

                int reqX = page->currentX;
                int reqY = page->lastRequestedY[reqX] - 1;

                qDebug() << "Alt+Down to" << reqX << "/" << reqY;

                page->requestFocus(reqX, reqY);
            } else {
                if (prevMsg.size() && prevIndex) {
                    prevIndex--;
                    textInput.setText(prevMsg.at(prevIndex));
                }
            }
        } else if (event->key() == Qt::Key_Down) {
            if (event->modifiers() == Qt::AltModifier) {
                NotebookPage *page = static_cast<NotebookPage *>(this->chatWidget->parentWidget());

                int reqX = page->currentX;
                int reqY = page->lastRequestedY[reqX] + 1;

                qDebug() << "Alt+Down to" << reqX << "/" << reqY;

                page->requestFocus(reqX, reqY);
            } else {
                if (prevIndex != (prevMsg.size() - 1) && prevIndex != prevMsg.size()) {
                    prevIndex++;
                    textInput.setText(prevMsg.at(prevIndex));
                } else {
                    prevIndex = prevMsg.size();
                    textInput.setText(QString());
                }
            }
        } else if (event->key() == Qt::Key_Left) {
            if (event->modifiers() == Qt::AltModifier) {
                NotebookPage *page = static_cast<NotebookPage *>(this->chatWidget->parentWidget());

                int reqX = page->currentX - 1;
                int reqY = page->lastRequestedY[reqX];

                qDebug() << "Alt+Left to" << reqX << "/" << reqY;

                page->requestFocus(reqX, reqY);
            }
        } else if (event->key() == Qt::Key_Right) {
            if (event->modifiers() == Qt::AltModifier) {
                NotebookPage *page = static_cast<NotebookPage *>(this->chatWidget->parentWidget());

                int reqX = page->currentX + 1;
                int reqY = page->lastRequestedY[reqX];

                qDebug() << "Alt+Right to" << reqX << "/" << reqY;

                page->requestFocus(reqX, reqY);
            }
        } else if (event->key() == Qt::Key_Tab) {
            if (event->modifiers() == Qt::ControlModifier) {
                NotebookPage *page = static_cast<NotebookPage *>(this->chatWidget->parentWidget());

                Notebook *notebook = static_cast<Notebook *>(page->parentWidget());

                notebook->nextTab();
            }
        } else if (event->key() == Qt::Key_Backtab) {
            if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
                NotebookPage *page = static_cast<NotebookPage *>(this->chatWidget->parentWidget());

                Notebook *notebook = static_cast<Notebook *>(page->parentWidget());

                notebook->previousTab();
            }
        }
    });

    this->textLengthVisibleChangedConnection =
        SettingsManager::getInstance().showMessageLength.valueChanged.connect(
            [this](const bool &value) { this->textLengthLabel.setHidden(!value); });
}

ChatWidgetInput::~ChatWidgetInput()
{
    this->textLengthVisibleChangedConnection.disconnect();
}

void ChatWidgetInput::refreshTheme()
{
    QPalette palette;

    palette.setColor(QPalette::Foreground, this->colorScheme.Text);

    this->textLengthLabel.setPalette(palette);

    this->textInput.setStyleSheet(this->colorScheme.InputStyleSheet);
}

void ChatWidgetInput::editTextChanged()
{
}

// void
// ChatWidgetInput::editKeyPressed(QKeyEvent *event)
//{
//    if (event->key() == Qt::Key_Enter) {
//        event->accept();
//        IrcManager::send("PRIVMSG #" +  edit.toPlainText();
//        edit.setText(QString());
//    }
//}

void ChatWidgetInput::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(this->rect(), this->colorScheme.ChatInputBackground);
    painter.setPen(this->colorScheme.ChatInputBorder);
    painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
}

void ChatWidgetInput::resizeEvent(QResizeEvent *)
{
    if (this->height() == this->maximumHeight()) {
        this->textInput.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    } else {
        this->textInput.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
}

void ChatWidgetInput::mousePressEvent(QMouseEvent *)
{
    this->chatWidget->giveFocus(Qt::MouseFocusReason);
}

}  // namespace widgets
}  // namespace chatterino
