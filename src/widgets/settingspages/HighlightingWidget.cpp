#include "widgets/settingspages/HighlightingWidget.hpp"

#include "common/QLogging.hpp"
#include "controllers/highlights/ConfigureDialog.hpp"
#include "controllers/highlights/Model.hpp"
#include "controllers/highlights/types/All.hpp"  // IWYU pragma: keep
#include "singletons/Settings.hpp"
#include "util/Variant.hpp"

#include <qabstractitemview.h>
#include <qboxlayout.h>
#include <qheaderview.h>
#include <qlabel.h>
#include <qmenu.h>
#include <qnamespace.h>
#include <qpushbutton.h>
#include <qstringlistmodel.h>
#include <qtableview.h>
#include <qtoolbutton.h>

#include <random>

namespace chatterino {

using namespace Qt::StringLiterals;

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoHighlights;

/// Tips that will be randomized once on launch and shown at the top of the settings highlights page
const std::vector<QStringView> TIPS{
    uR"(Filter highlights are handy for advanced highlights.)",
    uR"(User highlights highlights any message from the given user.)",
    uR"(You can edit a highlight by double-clicking it.)",
};

void moveRow(QTableView *tableView, int dir)
{
    assert(tableView != nullptr);
    assert(dir == 1 || dir == -1);

    auto *model = tableView->model();

    auto selection = tableView->selectionModel()->selectedRows(0);

    if (selection.isEmpty())
    {
        return;
    }

    int row = selection.at(0).row();
    if (row + dir >= model->rowCount())
    {
        return;
    }
    if (row + dir < 0)
    {
        return;
    }

    model->moveRows(model->index(row, 0), row, 1, model->index(row + dir, 0),
                    row + dir);
    tableView->selectRow(row + dir);
}

}  // namespace

HighlightingWidget::HighlightingWidget()
{
    auto *layout = new QVBoxLayout(this);

    auto *model = new highlights::Model(this);
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
                         qCDebug(LOG) << "Double-clicked row";
                         this->openConfigureDialog(
                             view, clicked, ConfigureCloseBehaviour::Cancel);
                         return;
                     });

    // view->resizeColumnsToContents();

    // view->horizontalHeader()->setStretchLastSection(true);
    view->horizontalHeader()->setSectionResizeMode(
        highlights::Model::Column::Enabled, QHeaderView::ResizeToContents);
    view->horizontalHeader()->setSectionResizeMode(
        highlights::Model::Column::Name, QHeaderView::ResizeToContents);

    auto *hints = new QHBoxLayout;
    layout->addLayout(hints);

    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(
            0, static_cast<int>(TIPS.size()) - 1);
        auto tip = TIPS.at(distrib(gen));
        hints->addWidget(
            new QLabel("Highlights are evaluated from top to bottom. " % tip));
    }

    auto *buttons = new QHBoxLayout();
    layout->addLayout(buttons);

    auto *add = new QPushButton("Add");
    auto *addMenu = new QMenu;
    addMenu->addAction(
        QIcon{":/buttons/text.svg"}, "Message highlight", [this, view] {
            auto id = highlights::generateID();
            highlights::MessageHighlight h{id};
            h.setPattern("my phrase");
            auto vectorIndex = getSettings()->sharedHighlights.append(h);
            this->openConfigureDialog(view, vectorIndex,
                                      ConfigureCloseBehaviour::Remove);
        });
    addMenu->addAction(
        QIcon{":/settings/accounts.svg"}, "User highlight", [this, view] {
            auto id = highlights::generateID();
            highlights::UserHighlight h{id};
            h.setUsername("highlighted user");
            auto vectorIndex = getSettings()->sharedHighlights.append(h);
            this->openConfigureDialog(view, vectorIndex,
                                      ConfigureCloseBehaviour::Remove);
        });
    addMenu->addAction("Badge highlight", [this, view] {
        auto id = highlights::generateID();
        highlights::BadgeHighlight h{id};
        h.setBadgeName("broadcaster");
        h.setDisplayName("Broadcaster");
        auto vectorIndex = getSettings()->sharedHighlights.append(h);
        this->openConfigureDialog(view, vectorIndex,
                                  ConfigureCloseBehaviour::Remove);
    });
    addMenu->addAction(
        QIcon{":/buttons/filters.svg"}, "Filter highlight", [this, view] {
            auto id = highlights::generateID();
            highlights::FilterHighlight h{id};
            h.setFilterText("message.content contains \"my phrase\"");
            auto vectorIndex = getSettings()->sharedHighlights.append(h);
            this->openConfigureDialog(view, vectorIndex,
                                      ConfigureCloseBehaviour::Remove);
        });
    add->setMenu(addMenu);
    buttons->addWidget(add);

    auto *edit = new QPushButton("Edit...");
    edit->setIcon(QIcon(":/buttons/edit.svg"));
    buttons->addWidget(edit);
    QObject::connect(edit, &QPushButton::clicked, this, [this, view] {
        auto selectedRow = view->selectionModel()->selectedRows(0);
        if (selectedRow.isEmpty())
        {
            qCWarning(LOG) << "Edit clicked with no selected row";
            return;
        }

        this->openConfigureDialog(view, selectedRow.at(0),
                                  ConfigureCloseBehaviour::Cancel);
    });
    edit->setEnabled(false);

    // move up
    auto *moveUp = new QPushButton("Move up");
    buttons->addWidget(moveUp);
    QObject::connect(moveUp, &QPushButton::clicked, this, [view] {
        moveRow(view, -1);
    });
    moveUp->setEnabled(false);

    // move down
    auto *moveDown = new QPushButton("Move down");
    buttons->addWidget(moveDown);
    QObject::connect(moveDown, &QPushButton::clicked, this, [view] {
        moveRow(view, 1);
    });
    moveDown->setEnabled(false);

    auto *remove = new QPushButton("Remove");
    buttons->addWidget(remove);
    QObject::connect(remove, &QPushButton::clicked, this, [view] {
        auto selectedRows = view->selectionModel()->selectedRows(0);
        if (selectedRows.isEmpty())
        {
            qCWarning(LOG) << "Remove clicked with no selected row";
            return;
        }
        auto selectedRow = selectedRows.at(0);
        assert(selectedRow.isValid());

        getSettings()->sharedHighlights.removeAt(selectedRow.row(), view);
    });
    remove->setEnabled(false);

    QObject::connect(
        view->selectionModel(), &QItemSelectionModel::selectionChanged, this,
        [=](const auto &selected, const auto &deselected) {
            auto selection = view->selectionModel()->selection();
            if (!selection.isEmpty())
            {
                qCDebug(LOG)
                    << "Model selection changed " << selected << deselected;

                edit->setDisabled(false);

                QModelIndex selectedItem = selection.indexes().at(0);
                assert(selectedItem.isValid());

                auto data =
                    selectedItem
                        .siblingAtColumn(highlights::Model::Column::Enabled)
                        .data(highlights::Model::DATA_ROLE)
                        .value<highlights::AllHighlights>();

                auto removable =
                    std::visit(variant::Overloaded{
                                   [](const highlights::MessageHighlight &) {
                                       return true;
                                   },
                                   [](const highlights::UserHighlight &) {
                                       return true;
                                   },
                                   [](const highlights::BadgeHighlight &) {
                                       return true;
                                   },
                                   [](const highlights::FilterHighlight &) {
                                       return true;
                                   },
                                   [](const auto &) {
                                       return false;
                                   },
                               },
                               data);
                remove->setDisabled(!removable);

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
                qCDebug(LOG) << "Model selection removed"
                             << view->selectionModel()->selection();
                edit->setDisabled(true);
                moveUp->setDisabled(true);
                moveDown->setDisabled(true);
                remove->setDisabled(true);
            }
        });

    layout->addWidget(view);
}

void HighlightingWidget::openConfigureDialog(
    QTableView *view, int vectorIndex,
    ConfigureCloseBehaviour newCloseBehaviour)
{
    auto index = view->model()->index(vectorIndex, 0);
    this->openConfigureDialog(view, index, newCloseBehaviour);
}

void HighlightingWidget::openConfigureDialog(
    QTableView *view, const QModelIndex &index,
    ConfigureCloseBehaviour newCloseBehaviour)
{
    if (this->configureDialog != nullptr)
    {
        return;
    }

    this->configureCloseBehaviour = newCloseBehaviour;

    auto data = index.siblingAtColumn(highlights::Model::Column::Enabled)
                    .data(highlights::Model::DATA_ROLE)
                    .value<highlights::AllHighlights>();

    auto id = highlights::getID(data);

    this->configureDialog = new highlights::ConfigureDialog(data, this);
    QObject::connect(
        this->configureDialog, &QWidget::destroyed, this, [this, id] {
            switch (this->configureCloseBehaviour)
            {
                case ConfigureCloseBehaviour::Cancel:
                    // do nothing
                    break;
                case ConfigureCloseBehaviour::Remove:
                    getSettings()->sharedHighlights.removeFirstMatching(
                        [id](const auto &h) -> bool {
                            auto incomingId = highlights::getID(h);
                            return incomingId == id;
                        },
                        this);
                    break;
            }

            this->configureDialog = nullptr;
        });

    QObject::connect(
        this->configureDialog, &highlights::ConfigureDialog::confirmed, this,
        [this, view, id](const highlights::AllHighlights &data) {
            // override close behaviour if user clicked ok
            this->configureCloseBehaviour = ConfigureCloseBehaviour::Cancel;

            // Find the index with the given ID since the model can have changed since we opened the configure dialog
            auto match = view->model()->match(
                view->model()->index(0, highlights::Model::Column::Enabled),
                highlights::Model::ID_ROLE, QVariant::fromValue(id), 1,
                Qt::MatchExactly | Qt::MatchWrap);
            if (match.isEmpty())
            {
                qCWarning(LOG) << "Attempted to edit" << id
                               << "but it is missing. Was it removed?";
                return;
            }

            view->model()->setItemData(match.at(0),
                                       {
                                           {
                                               highlights::Model::DATA_ROLE,
                                               QVariant::fromValue(data),
                                           },
                                       });
        });

    this->configureDialog->show();
}

}  // namespace chatterino
