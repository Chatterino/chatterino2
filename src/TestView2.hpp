#pragma once

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QPoint>
#include <QRect>
#include <QRegion>
#include <QWidget>

namespace chatterino {

class TestView2 : public QAbstractItemView
{
    Q_OBJECT

public:
    TestView2(QWidget *parent);

    QModelIndex indexAt(const QPoint &point) const override;
    void scrollTo(const QModelIndex &index,
                  QAbstractItemView::ScrollHint hint =
                      QAbstractItemView::ScrollHint::EnsureVisible) override;

    QRect visualRect(const QModelIndex &index) const override;

protected:
    int horizontalOffset() const override;
    int verticalOffset() const override;
    bool isIndexHidden(const QModelIndex &index) const override;
    QModelIndex moveCursor(QAbstractItemView::CursorAction cursorAction,
                           Qt::KeyboardModifiers modifiers) override;
    void setSelection(const QRect &rect,
                      QItemSelectionModel::SelectionFlags flags) override;
    QRegion visualRegionForSelection(
        const QItemSelection &selection) const override;
};

}  // namespace chatterino
