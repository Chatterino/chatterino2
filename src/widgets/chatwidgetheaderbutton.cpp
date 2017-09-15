#include "widgets/chatwidgetheaderbutton.hpp"
#include "colorscheme.hpp"
#include "widgets/chatwidgetheader.hpp"

#include <QBrush>
#include <QPainter>

namespace chatterino {
namespace widgets {

ChatWidgetHeaderButton::ChatWidgetHeaderButton(BaseWidget *parent, int spacing)
    : FancyButton(parent)
{
    setLayout(&this->ui.hbox);

    this->ui.label.setAlignment(Qt::AlignCenter);

    this->ui.hbox.setMargin(0);
    this->ui.hbox.addSpacing(spacing);
    this->ui.hbox.addWidget(&this->ui.label);
    this->ui.hbox.addSpacing(spacing);

    this->setMouseEffectColor(QColor(255, 255, 255, 63));
}

void ChatWidgetHeaderButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    this->fancyPaint(painter);
}

}  // namespace widgets
}  // namespace chatterino
