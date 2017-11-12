#include "widgets/helper/splitinput.hpp"
#include "colorscheme.hpp"
#include "completionmanager.hpp"
#include "ircmanager.hpp"
#include "settingsmanager.hpp"
#include "widgets/notebook.hpp"
#include "widgets/split.hpp"
#include "widgets/splitcontainer.hpp"

#include <QCompleter>
#include <QPainter>

namespace chatterino {
namespace widgets {

SplitInput::SplitInput(Split *_chatWidget, EmoteManager &emoteManager, WindowManager &windowManager)
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

    auto &fontManager = FontManager::getInstance();

    this->textInput.setFont(fontManager.getFont(FontManager::Type::Medium));
    fontManager.fontChanged.connect([this, &fontManager]() {
        this->textInput.setFont(fontManager.getFont(FontManager::Type::Medium));
    });

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

    connect(&textInput, &ResizingTextEdit::textChanged, this, &SplitInput::editTextChanged);

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
                SplitContainer *page =
                    static_cast<SplitContainer *>(this->chatWidget->parentWidget());

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
                SplitContainer *page =
                    static_cast<SplitContainer *>(this->chatWidget->parentWidget());

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

    this->textLengthVisibleChangedConnection =
        SettingsManager::getInstance().showMessageLength.valueChanged.connect(
            [this](const bool &value) { this->textLengthLabel.setHidden(!value); });

    QObject::connect(&this->textInput, &QTextEdit::copyAvailable, [this](bool available) {
        if (available) {
            this->chatWidget->view.clearSelection();
        }
    });
}

SplitInput::~SplitInput()
{
    this->textLengthVisibleChangedConnection.disconnect();
}

void SplitInput::clearSelection()
{
    QTextCursor c = this->textInput.textCursor();

    c.setPosition(c.position());
    c.setPosition(c.position(), QTextCursor::KeepAnchor);

    this->textInput.setTextCursor(c);
}

void SplitInput::refreshTheme()
{
    QPalette palette;

    palette.setColor(QPalette::Foreground, this->colorScheme.Text);

    this->textLengthLabel.setPalette(palette);

    this->textInput.setStyleSheet(this->colorScheme.InputStyleSheet);
}

void SplitInput::editTextChanged()
{
}

void SplitInput::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(this->rect(), this->colorScheme.ChatInputBackground);
    painter.setPen(this->colorScheme.ChatInputBorder);
    painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
}

void SplitInput::resizeEvent(QResizeEvent *)
{
    if (this->height() == this->maximumHeight()) {
        this->textInput.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    } else {
        this->textInput.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
}

void SplitInput::mousePressEvent(QMouseEvent *)
{
    this->chatWidget->giveFocus(Qt::MouseFocusReason);
}

}  // namespace widgets
}  // namespace chatterino
