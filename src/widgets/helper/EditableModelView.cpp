#include "EditableModelView.hpp"

#include <QAbstractTableModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

namespace chatterino {

EditableModelView::EditableModelView(QAbstractTableModel *model)
    : tableView_(new QTableView(this))
    , model_(model)
{
    this->model_->setParent(this);
    this->tableView_->setModel(model);
    this->tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    this->tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->tableView_->setDragDropMode(QTableView::DragDropMode::InternalMove);
    this->tableView_->setDragDropOverwriteMode(false);
    this->tableView_->setDefaultDropAction(Qt::DropAction::MoveAction);
    this->tableView_->verticalHeader()->setVisible(false);

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
    auto regexHelpLabel = new QLabel(
        "<a href='"
        "https://github.com/Chatterino/chatterino2/blob/master/docs/Regex.md'>"
        "<span style='color:#99f'>regex info</span></a>");
    regexHelpLabel->setOpenExternalLinks(true);
    this->addCustomButton(regexHelpLabel);
}

}  // namespace chatterino
