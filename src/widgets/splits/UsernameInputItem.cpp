#include "UsernameInputItem.hpp"

namespace chatterino {

UsernameInputItem::UsernameInputItem(const QString &text, ActionCallback action)
    : text_(text)
    , action_(action)
{
}

void UsernameInputItem::action()
{
    if (this->action_)
        this->action_(this->text_);
}

void UsernameInputItem::paint(QPainter *painter, const QRect &rect) const
{
    auto margin = 4;
    QRect textRect = QRect(rect.topLeft() + QPoint{margin, margin},
                           QSize(rect.width(), rect.height()));
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, this->text_);
}

QSize UsernameInputItem::sizeHint(const QRect &rect) const
{
    return QSize(rect.width(), ICON_SIZE.height());
}

}  // namespace chatterino
