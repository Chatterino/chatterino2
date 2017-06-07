#include "widgets/chatwidgetinput.h"
#include "chatwidget.h"
#include "colorscheme.h"
#include "ircmanager.h"
#include "settingsmanager.h"

#include <QCompleter>
#include <QPainter>
#include <boost/signals2.hpp>

namespace chatterino {
namespace widgets {

ChatWidgetInput::ChatWidgetInput(ChatWidget *widget)
    : _chatWidget(widget)
    , _hbox()
    , _vbox()
    , _editContainer()
    , _edit()
    , _textLengthLabel()
    , _emotesLabel(0)
{
    setLayout(&_hbox);
    setMaximumHeight(150);
    _hbox.setMargin(4);

    _hbox.addLayout(&_editContainer);
    _hbox.addLayout(&_vbox);

    _editContainer.addWidget(&_edit);
    _editContainer.setMargin(4);

    _vbox.addWidget(&_textLengthLabel);
    _vbox.addStretch(1);
    _vbox.addWidget(&_emotesLabel);

    _textLengthLabel.setText("100");
    _textLengthLabel.setAlignment(Qt::AlignRight);
    _emotesLabel.getLabel().setTextFormat(Qt::RichText);
    _emotesLabel.getLabel().setText(
        "<img src=':/images/Emoji_Color_1F60A_19.png' width='12' height='12' "
        "/>");

    QObject::connect(&_edit, &ResizingTextEdit::textChanged, this,
                     &ChatWidgetInput::editTextChanged);

    //    QObject::connect(&edit, &ResizingTextEdit::keyPressEvent, this,
    //                     &ChatWidgetInput::editKeyPressed);

    refreshTheme();
    setMessageLengthVisisble(SettingsManager::getInstance().showMessageLength.get());

    QStringList list;
    list.append("asd");
    list.append("asdf");
    list.append("asdg");
    list.append("asdh");

    QCompleter *completer = new QCompleter(list, &_edit);

    completer->setWidget(&_edit);

    _edit.keyPressed.connect([this /*, completer*/](QKeyEvent *event) {
        if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
            auto c = _chatWidget->getChannel();
            if (c == nullptr) {
                return;
            }

            c->sendMessage(_edit.toPlainText());
            event->accept();
            _edit.setText(QString());
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
                     &ChatWidgetInput::setMessageLengthVisisble);
                     */
}

ChatWidgetInput::~ChatWidgetInput()
{
    /* XXX(pajlada): FIX THIS
    QObject::disconnect(
        &Settings::getInstance().getShowMessageLength(),
        &BoolSetting::valueChanged, this,
        &ChatWidgetInput::setMessageLengthVisisble);
        */
}

void ChatWidgetInput::refreshTheme()
{
    QPalette palette;

    palette.setColor(QPalette::Foreground, ColorScheme::getInstance().Text);

    _textLengthLabel.setPalette(palette);

    _edit.setStyleSheet(ColorScheme::getInstance().InputStyleSheet);
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

    painter.fillRect(rect(), ColorScheme::getInstance().ChatInputBackground);
    painter.setPen(ColorScheme::getInstance().ChatInputBorder);
    painter.drawRect(0, 0, width() - 1, height() - 1);
}

void ChatWidgetInput::resizeEvent(QResizeEvent *)
{
    if (height() == maximumHeight()) {
        _edit.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    } else {
        _edit.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
}

}  // namespace widgets
}  // namespace chatterino
