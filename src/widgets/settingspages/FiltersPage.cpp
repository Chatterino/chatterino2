#include "FiltersPage.hpp"

#include "controllers/filters/FilterModel.hpp"
#include "singletons/Settings.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QTableView>

namespace chatterino {

FiltersPage::FiltersPage()
{
    LayoutCreator<FiltersPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    layout.emplace<QLabel>("Filter channels");
    EditableModelView *view =
        layout
            .emplace<EditableModelView>(
                (new FilterModel(nullptr))
                    ->initialized(&getSettings()->filterRecords))
            .getElement();

    view->setTitles({"Name", "Filter"});
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Fixed);
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);

    QTimer::singleShot(1, [view] {
        view->getTableView()->resizeColumnsToContents();
        view->getTableView()->setColumnWidth(0, 50);
        view->getTableView()->setColumnWidth(1, 300);
    });

    view->addButtonPressed.connect([] {
        auto next = QString::number(
            getSettings()->filterRecords.readOnly()->size() + 1);
        getSettings()->filterRecords.append(
            FilterRecord{QString("Filter %1").arg(next),
                         "message.content contains \"hello\""});
    });
}

void FiltersPage::onShow()
{
    return;
}

}  // namespace chatterino
