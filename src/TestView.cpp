#include "TestView.hpp"

namespace chatterino {

TestView::TestView(QWidget *parent)
    : QAbstractItemView(parent)
{
}

QModelIndex TestView::indexAt(const QPoint &point) const
{
    return {};
}

void TestView::scrollTo(const QModelIndex &index,
                        QAbstractItemView::ScrollHint hint)
{
}

QRect TestView::visualRect(const QModelIndex &index) const
{
    return {};
}

int TestView::horizontalOffset() const
{
    return {};
}

int TestView::verticalOffset() const
{
    return {};
}

bool TestView::isIndexHidden(const QModelIndex &index) const
{
    return {};
}

QModelIndex TestView::moveCursor(QAbstractItemView::CursorAction cursorAction,
                                 Qt::KeyboardModifiers modifiers)
{
    return {};
}

void TestView::setSelection(const QRect &rect,
                            QItemSelectionModel::SelectionFlags flags)
{
}

QRegion TestView::visualRegionForSelection(
    const QItemSelection &selection) const
{
    return {};
}

}  // namespace chatterino
