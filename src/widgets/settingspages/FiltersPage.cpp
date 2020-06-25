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

    view->setTitles({"Name", "Filter", "Valid"});
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Interactive);
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);

    QTimer::singleShot(1, [view] {
        view->getTableView()->resizeColumnsToContents();
        view->getTableView()->setColumnWidth(0, 150);
        view->getTableView()->setColumnWidth(2, 125);
    });

    view->addButtonPressed.connect([] {
        getSettings()->filterRecords.append(
            FilterRecord{"My filter", "message.content contains \"hello\""});
    });

    QObject::connect(view->getTableView(), &QTableView::clicked,
                     [this, view](const QModelIndex &clicked) {
                         this->tableCellClicked(clicked, view);
                     });
}

void FiltersPage::onShow()
{
    return;
}

void FiltersPage::tableCellClicked(const QModelIndex &clicked,
                                   EditableModelView *view)
{
    // valid column
    if (clicked.column() == 2)
    {
        QMessageBox popup;

        filterparser::FilterParser f(
            view->getModel()->data(clicked.siblingAtColumn(1)).toString());

        if (f.valid())
        {
            popup.setIcon(QMessageBox::Icon::Information);
            popup.setWindowTitle("Valid filter");
            popup.setText("Filter is valid");
            popup.setInformativeText(
                QString("Parsed as:\n%1").arg(f.debugString()));
        }
        else
        {
            popup.setIcon(QMessageBox::Icon::Warning);
            popup.setWindowTitle("Invalid filter");
            popup.setText(QString("Parsing errors occured:"));
            popup.setInformativeText(f.errors().join("\n"));
        }

        popup.exec();
    }
}

}  // namespace chatterino
