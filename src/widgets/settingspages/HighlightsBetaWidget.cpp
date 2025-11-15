#include "widgets/settingspages/HighlightsBetaWidget.hpp"

#include "controllers/highlights/HighlightBetaModel.hpp"
#include "singletons/Settings.hpp"
#include "widgets/dialogs/ColorPickerDialog.hpp"
#include "widgets/helper/color/ColorItemDelegate.hpp"
#include "widgets/settingspages/HighlightsBetaConfigureDelegate.hpp"
#include "widgets/settingspages/HighlightsBetaConfigureDialog.hpp"

#include <qabstractitemview.h>
#include <qboxlayout.h>
#include <qheaderview.h>
#include <qlabel.h>
#include <qstringlistmodel.h>
#include <qtableview.h>

namespace chatterino {

HighlightsBetaWidget::HighlightsBetaWidget()
{
    auto *layout = new QVBoxLayout(this);

    // auto *model = new QStringListModel({"a", "b", "c"});
    auto *model = new HighlightBetaModel(this);
    model->initialize(&getSettings()->highlightedMessages);

    QTableView *view = new QTableView(this);
    view->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setModel(model);
    // Disallow navigating between tabs or rows with tab
    view->setTabKeyNavigation(false);
    // view->setStyleSheet("*{color: red;} QTableView::item:focus{color: blue;}");
    // view->setCornerButtonEnabled(true);
    view->verticalHeader()->hide();
    view->horizontalHeader()->hide();
    //
    view->setItemDelegateForColumn(HighlightBetaModel::Column::Color,
                                   new ColorItemDelegate(view));
    view->setItemDelegateForColumn(HighlightBetaModel::Column::Configure,
                                   new HighlightsBetaConfigureDelegate(view));
    view->setMouseTracking(true);

    view->setShowGrid(false);

    QObject::connect(
        view, &QTableView::doubleClicked,
        [this, view, model](const QModelIndex &clicked) {
            if (clicked.column() == HighlightBetaModel::Column::Enabled)
            {
                // always ignore events from the "Enabled" checkbox column
                return;
            }

            // Open a dialog that references the model (qmodelindex?)
            // data that isn't directly
            qInfo() << "XXX: CONFIGURE HIGHLIGHT CUZ DOUBLE CLICK";
            this->openConfigureDialog(clicked);
            return;
        });

    QObject::connect(
        view, &QTableView::clicked,
        [this, view, model](const QModelIndex &clicked) {
            if (clicked.column() == HighlightBetaModel::Column::Color)
            {
                auto initial =
                    model->data(clicked, Qt::DecorationRole).value<QColor>();

                auto *dialog = new ColorPickerDialog(initial, this);
                // TODO: The QModelIndex clicked is technically not safe to persist here since the model
                // can be changed between the color dialog being created & the color dialog being closed
                QObject::connect(dialog, &ColorPickerDialog::colorConfirmed,
                                 this, [=](auto selected) {
                                     if (selected.isValid())
                                     {
                                         model->setData(clicked, selected,
                                                        Qt::DecorationRole);
                                     }
                                 });
                dialog->show();
                return;
            }

            if (clicked.column() == HighlightBetaModel::Column::Configure)
            {
                // Open a dialog that references the model (qmodelindex?)
                // data that isn't directly
                qInfo() << "XXX: CONFIGURE HIGHLIGHT";
                this->openConfigureDialog(clicked);
                return;
            }
        });

    // view->resizeColumnsToContents();

    // view->horizontalHeader()->setStretchLastSection(true);
    view->horizontalHeader()->setSectionResizeMode(
        HighlightBetaModel::Column::Enabled, QHeaderView::ResizeToContents);
    view->horizontalHeader()->setSectionResizeMode(
        HighlightBetaModel::Column::Name, QHeaderView::ResizeToContents);

    layout->addWidget(view);
}

void HighlightsBetaWidget::openConfigureDialog(const QModelIndex &index)
{
    if (this->configureDialog != nullptr)
    {
        qInfo() << "XXX: the configure dialog is already open";
        return;
    }

    HighlightData data;  // TODO: fill this in based on the index

    qInfo() << "XXX: show?";
    this->configureDialog = new HighlightsBetaConfigureDialog(data, this);
    QObject::connect(this->configureDialog, &QWidget::destroyed, this, [this] {
        qInfo() << "XXX: closed";
        this->configureDialog = nullptr;
    });

    this->configureDialog->show();
}

}  // namespace chatterino
