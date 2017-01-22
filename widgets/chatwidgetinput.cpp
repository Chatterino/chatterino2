#include "widgets/chatwidgetinput.h"
#include "colorscheme.h"

#include <QPainter>

namespace chatterino {
namespace widgets {

ChatWidgetInput::ChatWidgetInput()
    : hbox()
    , vbox()
    , editContainer()
    , edit()
    , textLengthLabel()
    , emotesLabel(0)
{
    this->setLayout(&this->hbox);
    this->setMaximumHeight(150);
    this->hbox.setMargin(4);

    this->hbox.addLayout(&this->editContainer);
    this->hbox.addLayout(&this->vbox);

    this->editContainer.addWidget(&this->edit);
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

    //    this->emotesLabel.setMaximumSize(12, 12);

    this->refreshTheme();
}

void
ChatWidgetInput::refreshTheme()
{
    QPalette palette;

    palette.setColor(QPalette::Foreground, ColorScheme::instance().Text);

    this->textLengthLabel.setPalette(palette);

    edit.setStyleSheet(ColorScheme::instance().InputStyleSheet);
}

void
ChatWidgetInput::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), ColorScheme::instance().ChatInputBackground);
    painter.setPen(ColorScheme::instance().ChatInputBorder);
    painter.drawRect(0, 0, width() - 1, height() - 1);
}

void
ChatWidgetInput::resizeEvent(QResizeEvent *)
{
    if (height() == this->maximumHeight()) {
        edit.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    } else {
        edit.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
}
}
}
