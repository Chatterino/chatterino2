#pragma once

#include <QTableView>

class QAbstractTableModel;
class QDragEnterEvent;
class QDropEvent;
class QPoint;

namespace chatterino {

class EditableTableView : public QTableView
{
public:
    EditableTableView(QAbstractTableModel *model);

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
    QAbstractTableModel *model_{};
    int dragEnteredY_;

    int row(int viewportY);
};

}  // namespace chatterino
