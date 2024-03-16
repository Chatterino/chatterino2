#include "widgets/helper/TableStyle.hpp"

namespace chatterino {

TableStyle::TableStyle(QStyle *target)
    : QProxyStyle(target)
{
}

void TableStyle::drawPrimitive(QStyle::PrimitiveElement element,
                               const QStyleOption *option, QPainter *painter,
                               const QWidget *widget) const
{
    if (element != QStyle::PE_IndicatorItemViewItemDrop)
    {
        QProxyStyle::drawPrimitive(element, option, painter, widget);
        return;
    }

    const auto *view = dynamic_cast<const QAbstractItemView *>(widget);
    if (!view)
    {
        assert(false && "TableStyle must be used on a QAbstractItemView");
        return;
    }

    if (option->rect.isNull())
    {
        painter->setPen({Qt::red, 1});
        painter->drawRect(view->viewport()->rect().adjusted(0, 0, -1, -1));
        return;
    }

    // Get the direction a row is dragged in
    auto selected = view->currentIndex();
    auto hovered = view->indexAt(option->rect.center());
    if (!selected.isValid() || !hovered.isValid())
    {
        // This shouldn't happen as we're in a drag operation
        assert(false && "Got bad indices");
        return;
    }

    int y = option->rect.top();  // move up
    if (hovered.row() >= selected.row())
    {
        y = option->rect.bottom();  // move down
    }

    painter->setPen({Qt::white, 2});
    painter->drawLine(0, y, widget->width(), y);
}

}  // namespace chatterino
