#include "EditableModelView.hpp"

#include <QAbstractTableModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

namespace chatterino
{
    EditableModelView::EditableModelView(QAbstractTableModel* model)
        : tableView_(new QTableView(this))
        , model_(model)
    {
        this->model_->setParent(this);
        this->tableView_->setModel(model);
        this->tableView_->setSelectionMode(
            QAbstractItemView::ExtendedSelection);
        this->tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
        this->tableView_->verticalHeader()->hide();

        // create layout
        QVBoxLayout* vbox = new QVBoxLayout(this);
        vbox->setMargin(0);
        vbox->addWidget(this->tableView_);

        // create button layout
        QHBoxLayout* buttons = new QHBoxLayout(this);
        vbox->addLayout(buttons);

        // add
        QPushButton* add = new QPushButton("Add");
        buttons->addWidget(add);
        QObject::connect(add, &QPushButton::clicked,
            [this] { this->addButtonPressed.invoke(); });

        // remove
        QPushButton* remove = new QPushButton("Remove");
        buttons->addWidget(remove);
        QObject::connect(remove, &QPushButton::clicked, [this] {
            QModelIndexList list;
            while (
                (list = this->getTableView()->selectionModel()->selectedRows(0))
                    .length() > 0)
            {
                model_->removeRow(list[0].row());
            }
        });

        // finish button layout
        buttons->addStretch(1);
    }

    void EditableModelView::setTitles(std::initializer_list<QString> titles)
    {
        int i = 0;
        for (const QString& title : titles)
        {
            if (this->model_->columnCount() == i)
            {
                break;
            }

            this->model_->setHeaderData(
                i++, Qt::Horizontal, title, Qt::DisplayRole);
        }
    }

    QTableView* EditableModelView::getTableView()
    {
        return this->tableView_;
    }

    QAbstractTableModel* EditableModelView::getModel()
    {
        return this->model_;
    }

}  // namespace chatterino
