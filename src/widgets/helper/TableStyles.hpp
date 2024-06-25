#pragma once

#include <QProxyStyle>

class QTableView;

namespace chatterino {

/// @brief A custom style for drag operations of rows on tables
///
/// This style overwrites how `PE_IndicatorItemViewItemDrop`, the drop
/// indicator of item-views, is drawn. It's intended to be used on QTableViews
/// where entire rows are moved (not individual cells). The indicator is shown
/// as a line at the position where the dragged item should be inserted. If no
/// such position exists, a red border is drawn around the viewport.
class TableRowDragStyle : public QProxyStyle
{
public:
    /// Applies the style to @a view
    static void applyTo(QTableView *view);

    void drawPrimitive(QStyle::PrimitiveElement element,
                       const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = nullptr) const override;

private:
    /// @param name The style name to emulate.
    ///             This should be set to `style()->name()`.
    TableRowDragStyle(const QString &name);
};

}  // namespace chatterino
