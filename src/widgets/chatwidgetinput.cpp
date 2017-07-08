#include "widgets/chatwidgetinput.hpp"
#include "chatwidget.hpp"
#include "colorscheme.hpp"
#include "ircmanager.hpp"
#include "settingsmanager.hpp"

#include <QCompleter>
#include <QPainter>
#include <boost/signals2.hpp>

namespace chatterino {
namespace widgets {

ChatWidgetInput::ChatWidgetInput(ChatWidget *_chatWidget)
    : BaseWidget(_chatWidget)
    , chatWidget(_chatWidget)
    , emotesLabel(this)
{
    this->setMaximumHeight(150);

    this->setLayout(&this->hbox);

    this->hbox.setMargin(4);

    this->hbox.addLayout(&this->editContainer);
    this->hbox.addLayout(&this->vbox);

    this->editContainer.addWidget(&this->textInput);
    this->editContainer.setMargin(4);

    this->vbox.addWidget(&this->textLengthLabel);
    this->vbox.addStretch(1);
    this->vbox.addWidget(&this->emotesLabel);

    this->textLengthLabel.setText("100");
    this->textLengthLabel.setAlignment(Qt::AlignRight);

    this->emotesLabel.getLabel().setTextFormat(Qt::RichText);
    this->emotesLabel.getLabel().setText(
        "<img src=':/images/Emoji_Color_1F60A_19.png' width='12' height='12' "
        "/>");

    connect(&textInput, &ResizingTextEdit::textChanged, this, &ChatWidgetInput::editTextChanged);

    this->refreshTheme();
    this->setMessageLengthVisible(SettingsManager::getInstance().showMessageLength.get());

    QStringList list;
    /*list.append("asd");
    list.append("asdf");
    list.append("asdg");
    list.append("asdh");
*/
    list << "Kappa" << "asd" << "asdf" << "asdg";
    QCompleter *completer = new QCompleter(list, &this->textInput);

    this->textInput.setCompleter(completer);

    this->textInput.keyPressed.connect([this /*, completer*/](QKeyEvent *event) {
        if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
            auto c = this->chatWidget->getChannel();
            if (c == nullptr) {
                return;
            }

            c->sendMessage(textInput.toPlainText());
            event->accept();
            textInput.setText(QString());
        }
        //        else {
        //            completer->setCompletionPrefix("asdf");
        //            completer->complete();
        //            //            completer->popup();
        //        }
    });

    /* XXX(pajlada): FIX THIS
    QObject::connect(&Settings::getInstance().showMessageLength,
                     &BoolSetting::valueChanged, this,
                     &ChatWidgetInput::setMessageLengthVisible);
                     */
}

ChatWidgetInput::~ChatWidgetInput()
{
    /* XXX(pajlada): FIX THIS
    QObject::disconnect(
        &Settings::getInstance().getShowMessageLength(),
        &BoolSetting::valueChanged, this,
        &ChatWidgetInput::setMessageLengthVisible);
        */
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

}  // namespace widgets
}  // namespace chatterino
