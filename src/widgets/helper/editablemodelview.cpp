#include "editablemodelview.hpp"

#include <QAbstractTableModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

namespace chatterino {
namespace widgets {
namespace helper {

EditableModelView::EditableModelView(QAbstractTableModel *_model)
    : tableView(new QTableView(this))
    , model(_model)
{
    this->model->setParent(this);
    this->tableView->setModel(_model);
    this->tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    this->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->tableView->verticalHeader()->hide();

    // create layout
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    vbox->addWidget(this->tableView);

    // create button layout
    QHBoxLayout *buttons = new QHBoxLayout(this);
    vbox->addLayout(buttons);

    // add
    QPushButton *add = new QPushButton("Add");
    buttons->addWidget(add);
    QObject::connect(add, &QPushButton::clicked, [this] { this->addButtonPressed.invoke(); });

    // remove
    QPushButton *remove = new QPushButton("Remove");
    buttons->addWidget(remove);
    QObject::connect(remove, &QPushButton::clicked, [this] {
        QModelIndexList list;
        while ((list = this->getTableView()->selectionModel()->selectedRows(0)).length() > 0) {
            model->removeRow(list[0].row());
        }
    });

    // finish button layout
    buttons->addStretch(1);
}

void EditableModelView::setTitles(std::initializer_list<QString> titles)
{
    int i = 0;
    for (const QString &title : titles) {
        if (this->model->columnCount() == i) {
            break;
        }

        this->model->setHeaderData(i++, Qt::Horizontal, title, Qt::DisplayRole);
    }
}

QTableView *EditableModelView::getTableView()
{
    return this->tableView;
}

QAbstractTableModel *EditableModelView::getModel()
{
    return this->model;
}

}  // namespace helper
}  // namespace widgets
}  // namespace chatterino
