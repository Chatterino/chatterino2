#include "AliasesPage.hpp"

#include "controllers/aliases/AliasesModel.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Window.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QTableView>

#include <QHeaderView>

namespace chatterino {

AliasesPage::AliasesPage()
{
    LayoutCreator<AliasesPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    layout.emplace<QLabel>(
        "Selectively display messages in Splits using channel filters. Set "
        "filters under a Split menu.");
    EditableModelView *view =
        layout
            .emplace<EditableModelView>(
                (new AliasesModel(nullptr))
                    ->initialized(&getSettings()->aliasNames))
            .getElement();

    view->setTitles({"Name", "Replacement"});
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Interactive);
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);

    view->addButtonPressed.connect([] {
        getSettings()->aliasNames.append(
            std::make_shared<AliasesName>("Name", "", "Replacement"));
    });

    QTimer::singleShot(1, [view] {
        view->getTableView()->resizeColumnsToContents();
        view->getTableView()->setColumnWidth(0, 250);
    });

    QObject::connect(view->getTableView(), &QTableView::clicked,
                     [this, view](const QModelIndex &clicked) {
                         this->tableCellClicked(clicked, view);
                     });
}

void AliasesPage::tableCellClicked(const QModelIndex &clicked,
                                   EditableModelView *view)
{
}

}  // namespace chatterino
