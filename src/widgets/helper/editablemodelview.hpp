#pragma once

#include <QWidget>

#include <pajlada/signals/signal.hpp>

class QAbstractTableModel;
class QTableView;

namespace chatterino {
namespace widgets {
namespace helper {

class EditableModelView : public QWidget
{
public:
    EditableModelView(QAbstractTableModel *model);

    void setTitles(std::initializer_list<QString> titles);

    QTableView *getTableView();
    QAbstractTableModel *getModel();

    pajlada::Signals::NoArgSignal addButtonPressed;

private:
    QTableView *tableView;
    QAbstractTableModel *model;
};

}  // namespace helper
}  // namespace widgets
}  // namespace chatterino
