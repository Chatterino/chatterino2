#include "TestView2.hpp"

namespace chatterino {

TestView2::TestView2(QWidget *parent)
    : QAbstractItemView(parent)
{
}

QModelIndex TestView2::indexAt(const QPoint &point) const
{
    return {};
}

void TestView2::scrollTo(const QModelIndex &index,
                         QAbstractItemView::ScrollHint hint)
{
}

QRect TestView2::visualRect(const QModelIndex &index) const
{
    return {};
}

int TestView2::horizontalOffset() const
{
    return {};
}

int TestView2::verticalOffset() const
{
    return {};
}

bool TestView2::isIndexHidden(const QModelIndex &index) const
{
    return {};
}

QModelIndex TestView2::moveCursor(QAbstractItemView::CursorAction cursorAction,
                                  Qt::KeyboardModifiers modifiers)
{
    return {};
}

void TestView2::setSelection(const QRect &rect,
                             QItemSelectionModel::SelectionFlags flags)
{
}

QRegion TestView2::visualRegionForSelection(
    const QItemSelection &selection) const
{
    return {};
}

}  // namespace chatterino
