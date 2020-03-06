#pragma once

#include <QWidget>

#include <pajlada/signals/signal.hpp>

class QAbstractTableModel;
class QTableView;
class QHBoxLayout;

namespace chatterino {

class EditableModelView : public QWidget
{
public:
    EditableModelView(QAbstractTableModel *model);

    void setTitles(std::initializer_list<QString> titles);

    QTableView *getTableView();
    QAbstractTableModel *getModel();

    pajlada::Signals::NoArgSignal addButtonPressed;

    void addCustomButton(QWidget *widget);
    void addRegexHelpLink();

private:
    QTableView *tableView_{};
    QAbstractTableModel *model_{};
    QHBoxLayout *buttons_{};

    void moveRow(int dir);
};

}  // namespace chatterino
