#include "NicknamesPage.hpp"

#include "controllers/nicknames/NicknamesModel.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Window.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QTableView>

#include <QHeaderView>

namespace chatterino {

NicknamesPage::NicknamesPage()
{
    LayoutCreator<NicknamesPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    layout.emplace<QLabel>(
        "Nicknames do not work with features such as search or user highlights."
        "\nWith those features you will still need to use the user's original "
        "name.");
    EditableModelView *view =
        layout
            .emplace<EditableModelView>(
                (new NicknamesModel(nullptr))
                    ->initialized(&getSettings()->nicknames))
            .getElement();

    view->setTitles({"Username", "Nickname"});
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Interactive);
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);

    view->addButtonPressed.connect([] {
        getSettings()->nicknames.append(Nickname{"Username", "Nickname"});
    });

    QTimer::singleShot(1, [view] {
        view->getTableView()->resizeColumnsToContents();
        view->getTableView()->setColumnWidth(0, 250);
    });
}

}  // namespace chatterino
