#include "widgets/dialogs/switcher/AbstractSwitcherItem.hpp"

#include "Application.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Theme.hpp"

namespace chatterino {

const int PaintingScaleFactor = 20;
const QSize iconSize(32, 32);

AbstractSwitcherItem::AbstractSwitcherItem(const QString &text)
    : text_(text)
{
}

AbstractSwitcherItem::AbstractSwitcherItem(const QIcon &icon,
                                           const QString &text)
    : icon_(icon)
    , text_(text)
{
}

void AbstractSwitcherItem::paint(QPainter *painter, const QRect &rect) const
{
    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, true);

    // TODO(leon): Right pen/brush/font settings?
    painter->setPen(getApp()->themes->splits.header.text);
    painter->setBrush(Qt::SolidPattern);
    painter->setFont(getApp()->fonts->getFont(FontStyle::UiMediumBold, 1.0));

    QRect iconRect(rect.topLeft(), iconSize);
    this->icon_.paint(painter, iconRect, Qt::AlignLeft | Qt::AlignVCenter);

    QRect textRect =
        QRect(iconRect.topRight(),
              QSize(rect.width() - iconRect.width(), iconRect.height()));
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, this->text_);
    painter->scale(PaintingScaleFactor, PaintingScaleFactor);

    painter->restore();
}

QSize AbstractSwitcherItem::sizeHint(const QRect &rect) const
{
    return QSize(rect.width(), iconSize.height());
}

}  // namespace chatterino
