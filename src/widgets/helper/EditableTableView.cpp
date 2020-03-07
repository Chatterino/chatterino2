#include "EditableTableView.hpp"

#include <QAbstractItemView>
#include <QAbstractTableModel>
#include <QDragEnterEvent>
#include <QDropEvent>

namespace chatterino {

EditableTableView::EditableTableView(QAbstractTableModel *model)
{
    this->model_ = model;
    this->setModel(model);
    this->setSelectionMode(QAbstractItemView::SingleSelection);
    this->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->setDragDropMode(QTableView::DragDropMode::InternalMove);
    this->setDragDropOverwriteMode(true);
    this->setDefaultDropAction(Qt::DropAction::MoveAction);
    this->verticalHeader()->setVisible(false);
    this->setDragEnabled(true);
    this->setAcceptDrops(true);
}

void EditableTableView::dragEnterEvent(QDragEnterEvent *event)
{
    // TODO: Improve grabbing dragEnteredY
    //       It takes a couple of pixels of dragging for dragEnterEvent to trigger,
    //       which can sometimes cause the wrong row to be moved when dragging on the edge of a row.
    this->dragEnteredY_ = event->pos().y();
    event->accept();
}

void EditableTableView::dropEvent(QDropEvent *event)
{
    int dragRow = this->row(this->dragEnteredY_);
    int dropRow = this->row(event->pos().y());

    this->model_->moveRow(this->model_->index(dragRow, 0), dragRow,
                          this->model_->index(dropRow, 0), dropRow);
    this->selectionModel()->clear();
    this->selectionModel()->select(
        model_->index(dropRow, 0),
        QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
}

int EditableTableView::row(int viewportY)
{
    for (int i = 0; i < this->model_->rowCount(); ++i)
    {
        int currentViewportY = this->rowViewportPosition(i);
        if (viewportY >= currentViewportY &&
            viewportY <= currentViewportY + this->rowHeight(i))
            return i;
    }
    return -1;
}

}  // namespace chatterino
