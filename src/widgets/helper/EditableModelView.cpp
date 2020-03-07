#include "EditableModelView.hpp"
#include "EditableTableView.hpp"

#include <QAbstractItemView>
#include <QAbstractTableModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QModelIndex>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

namespace chatterino {

EditableModelView::EditableModelView(QAbstractTableModel *model)
{
    this->model_ = model;
    this->model_->setParent(this);
    this->tableView_ = new EditableTableView(model_);

    // create layout
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);

    // create button layout
    QHBoxLayout *buttons = new QHBoxLayout(this);
    this->buttons_ = buttons;
    vbox->addLayout(buttons);

    // add
    QPushButton *add = new QPushButton("Add");
    buttons->addWidget(add);
    QObject::connect(add, &QPushButton::clicked,
                     [this] { this->addButtonPressed.invoke(); });

    // remove
    QPushButton *remove = new QPushButton("Remove");
    buttons->addWidget(remove);
    QObject::connect(remove, &QPushButton::clicked, [this] {
        auto selected = this->getTableView()->selectionModel()->selectedRows(0);

        // Remove rows backwards so indices don't shift.
        std::vector<int> rows;
        for (auto &&index : selected)
            rows.push_back(index.row());

        std::sort(rows.begin(), rows.end(), std::greater{});

        for (auto &&row : rows)
            model_->removeRow(row);
    });

    // move up
    QPushButton *moveUp = new QPushButton("Move up");
    buttons->addWidget(moveUp);
    QObject::connect(moveUp, &QPushButton::clicked,
                     [this] { this->moveRow(-1); });

    // move down
    QPushButton *moveDown = new QPushButton("Move down");
    buttons->addWidget(moveDown);
    QObject::connect(moveDown, &QPushButton::clicked,
                     [this] { this->moveRow(1); });

    buttons->addStretch();

    // add tableview
    vbox->addWidget(this->tableView_);

    // finish button layout
    buttons->addStretch(1);
}

void EditableModelView::setTitles(std::initializer_list<QString> titles)
{
    int i = 0;
    for (const QString &title : titles)
    {
        if (this->model_->columnCount() == i)
        {
            break;
        }

        this->model_->setHeaderData(i++, Qt::Horizontal, title,
                                    Qt::DisplayRole);
    }
}

QTableView *EditableModelView::getTableView()
{
    return this->tableView_;
}

QAbstractTableModel *EditableModelView::getModel()
{
    return this->model_;
}

void EditableModelView::addCustomButton(QWidget *widget)
{
    this->buttons_->addWidget(widget);
}

void EditableModelView::addRegexHelpLink()
{
    auto regexHelpLabel =
        new QLabel("<a href='https://chatterino.com/help/regex'><span "
                   "style='color:#99f'>regex info</span></a>");
    regexHelpLabel->setOpenExternalLinks(true);
    this->addCustomButton(regexHelpLabel);
}

void EditableModelView::moveRow(int dir)
{
    auto selected = this->getTableView()->selectionModel()->selectedRows(0);

    int row;
    if (selected.size() == 0 ||
        (row = selected.at(0).row()) + dir >=
            this->model_->rowCount(QModelIndex()) ||
        row + dir < 0)
        return;

    model_->moveRows(model_->index(row, 0), row, selected.size(),
                     model_->index(row + dir, 0), row + dir);

    this->getTableView()->selectionModel()->clear();
    this->getTableView()->selectionModel()->select(
        model_->index(row + dir, 0),
        QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
}

}  // namespace chatterino
