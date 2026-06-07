#include "widgets/settingspages/HighlightsBetaWidget.hpp"

#include "controllers/highlights/HighlightBetaModel.hpp"
#include "singletons/Settings.hpp"
#include "widgets/settingspages/HighlightsBetaConfigureDelegate.hpp"
#include "widgets/settingspages/HighlightsBetaConfigureDialog.hpp"

#include <qabstractitemview.h>
#include <qboxlayout.h>
#include <qheaderview.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qstringlistmodel.h>
#include <qtableview.h>
#include <qtoolbutton.h>

namespace chatterino {

using namespace Qt::StringLiterals;

namespace {

void moveRow(QTableView *tableView, int dir)
{
    assert(tableView != nullptr);
    assert(dir == 1 || dir == -1);

    auto *model = tableView->model();

    auto selected = tableView->selectionModel()->selectedRows(0);

    int row;
    if (selected.size() == 0 ||
        (row = selected.at(0).row()) + dir >= model->rowCount(QModelIndex()) ||
        row + dir < 0)
    {
        return;
    }

    model->moveRows(model->index(row, 0), row, selected.size(),
                    model->index(row + dir, 0), row + dir);
    tableView->selectRow(row + dir);
}

}  // namespace

HighlightsBetaWidget::HighlightsBetaWidget()
{
    auto *layout = new QVBoxLayout(this);

    // auto *model = new QStringListModel({"a", "b", "c"});
    auto *model = new HighlightBetaModel(this);
    model->setObjectName("New Highlight Model");
    model->initialize(&getSettings()->sharedHighlights);

    QTableView *view = new QTableView(this);
    // view->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
    view->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setModel(model);
    // Disallow navigating between tabs or rows with tab
    view->setTabKeyNavigation(false);
    // view->setStyleSheet("*{color: red;} QTableView::item:focus{color: blue;}");
    // view->setCornerButtonEnabled(true);
    view->verticalHeader()->hide();
    view->horizontalHeader()->hide();
    //
    view->setMouseTracking(true);

    view->setShowGrid(false);

    QObject::connect(view, &QTableView::doubleClicked,
                     [this, view](const QModelIndex &clicked) {
                         // Open a dialog that references the model (qmodelindex?)
                         // data that isn't directly
                         qInfo() << "XXX: CONFIGURE HIGHLIGHT CUZ DOUBLE CLICK";
                         this->openConfigureDialog(view, clicked);
                         return;
                     });

    QObject::connect(view, &QTableView::clicked,
                     [this, view, model](const QModelIndex &clicked) {
                         //
                     });

    // view->resizeColumnsToContents();

    // view->horizontalHeader()->setStretchLastSection(true);
    view->horizontalHeader()->setSectionResizeMode(
        HighlightBetaModel::Column::Enabled, QHeaderView::ResizeToContents);
    view->horizontalHeader()->setSectionResizeMode(
        HighlightBetaModel::Column::Name, QHeaderView::ResizeToContents);

    auto *hints = new QHBoxLayout;
    layout->addLayout(hints);

    hints->addWidget(
        new QLabel("To edit a highlight, select it by clicking it once then "
                   "click the \"Edit\" button, or double click it."));

    auto *buttons = new QHBoxLayout();
    layout->addLayout(buttons);

    auto *edit = new QPushButton("Edit...");
    edit->setIcon(QIcon(":/buttons/edit.svg"));
    buttons->addWidget(edit);
    QObject::connect(edit, &QPushButton::clicked, this, [view] {
        moveRow(view, -1);
    });

    // move up
    auto *moveUp = new QPushButton("Move up");
    buttons->addWidget(moveUp);
    QObject::connect(moveUp, &QPushButton::clicked, this, [view] {
        moveRow(view, -1);
    });

    // move down
    auto *moveDown = new QPushButton("Move down");
    buttons->addWidget(moveDown);
    QObject::connect(moveDown, &QPushButton::clicked, this, [view] {
        moveRow(view, 1);
    });

    QObject::connect(
        view->selectionModel(), &QItemSelectionModel::selectionChanged, this,
        [=](const auto &selected, const auto &deselected) {
            if (!selected.isEmpty())
            {
                qInfo() << "XXX: SELECTION CHANGED" << selected << deselected;
                edit->setDisabled(false);

                auto selectedItem = selected.indexes().at(0);
                if (selectedItem.row() == 0)
                {
                    moveUp->setDisabled(true);
                }
                else
                {
                    moveUp->setDisabled(false);
                }

                if (selectedItem.row() == view->model()->rowCount() - 1)
                {
                    moveDown->setDisabled(true);
                }
                else
                {
                    moveDown->setDisabled(false);
                }
            }
            else
            {
                qInfo() << "XXX: Selection removed";
                edit->setDisabled(true);
                moveUp->setDisabled(true);
                moveDown->setDisabled(true);
            }
        });

    layout->addWidget(view);
}

void HighlightsBetaWidget::openConfigureDialog(QTableView *view,
                                               const QModelIndex &index)
{
    if (this->configureDialog != nullptr)
    {
        qInfo() << "XXX: the configure dialog is already open";
        return;
    }

    auto data =
        index.siblingAtColumn(HighlightBetaModel::Column::Enabled)
            .data(HighlightBetaModel::DATA_ROLE)
            .value<SharedHighlight>();  // TODO: fill this in based on the index

    qInfo() << "XXX: show?" << data;
    this->configureDialog = new HighlightsBetaConfigureDialog(data, this);
    QObject::connect(this->configureDialog, &QWidget::destroyed, this, [this] {
        qInfo() << "XXX: closed";
        this->configureDialog = nullptr;
    });
    QObject::connect(
        this->configureDialog, &HighlightsBetaConfigureDialog::confirmed, this,
        [this, index, view](SharedHighlight data) {
            qInfo() << "XXX: confirmed" << data;
            view->model()->setItemData(
                index.siblingAtColumn(HighlightBetaModel::Column::Sound),
                {
                    {
                        HighlightBetaModel::DATA_ROLE,
                        QVariant::fromValue(data),
                    },
                });
            view->model()->setItemData(
                index.siblingAtColumn(HighlightBetaModel::Column::Enabled),
                {
                    {
                        HighlightBetaModel::DATA_ROLE,
                        QVariant::fromValue(data),
                    },
                });
        });

    this->configureDialog->show();
}

}  // namespace chatterino
