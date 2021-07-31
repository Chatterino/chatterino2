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
        "Give an alias to a user which changes their name in chat for "
        "you.\nThis does not work with features such as search, in those "
        "places you will still need to use the user's original name.");
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
        getSettings()->aliasNames.append(AliasesName{"Name", "Replacement"});
    });

    QTimer::singleShot(1, [view] {
        view->getTableView()->resizeColumnsToContents();
        view->getTableView()->setColumnWidth(0, 250);
    });
}

}  // namespace chatterino
