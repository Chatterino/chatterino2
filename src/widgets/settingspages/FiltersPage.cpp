#include "widgets/settingspages/FiltersPage.hpp"

#include "Application.hpp"
#include "controllers/filters/FilterModel.hpp"
#include "controllers/filters/FilterRecord.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/dialogs/ChannelFilterEditorDialog.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QHeaderView>
#include <QTableView>

namespace chatterino {

constexpr QStringView FILTERS_DOCUMENTATION =
    u"https://wiki.chatterino.com/Filters";

FiltersPage::FiltersPage()
{
    LayoutCreator<FiltersPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    layout.emplace<QLabel>(
        "Selectively display messages in Splits using channel filters. Set "
        "filters under a Split menu.");
    this->view_ = layout
                      .emplace<EditableModelView>(
                          (new FilterModel(nullptr))
                              ->initialized(&getSettings()->filterRecords))
                      .getElement();

    this->view_->setTitles({"Name", "Filter", "Valid"});
    this->view_->getTableView()->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Interactive);
    this->view_->getTableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);

    QTimer::singleShot(1, [this] {
        this->view_->getTableView()->resizeColumnsToContents();
        this->view_->getTableView()->setColumnWidth(0, 150);
        this->view_->getTableView()->setColumnWidth(2, 125);
    });

    // We can safely ignore this signal connection since we own the this->view_
    std::ignore = this->view_->addButtonPressed.connect([this] {
        ChannelFilterEditorDialog d(this->window());
        if (d.exec() == QDialog::Accepted)
        {
            getSettings()->filterRecords.append(
                std::make_shared<FilterRecord>(d.getTitle(), d.getFilter()));
        }
    });

    auto *quickAddButton = new QPushButton("Quick Add");
    QObject::connect(quickAddButton, &QPushButton::pressed, [] {
        getSettings()->filterRecords.append(std::make_shared<FilterRecord>(
            "My filter", "message.content contains \"hello\""));
    });
    this->view_->addCustomButton(quickAddButton);

    QObject::connect(view_->getTableView(), &QTableView::clicked,
                     [this](const QModelIndex &clicked) {
                         this->tableCellClicked(clicked, this->view_);
                     });

    auto *filterHelpLabel =
        new QLabel(QStringView(u"<a href='%1'><span "
                               "style='color:#99f'>filter info</span></a>")
                       .arg(FILTERS_DOCUMENTATION));
    filterHelpLabel->setOpenExternalLinks(true);
    this->view_->addCustomButton(filterHelpLabel);

    layout.append(
        this->createCheckBox("Do not filter my own messages",
                             getSettings()->excludeUserMessagesFromFilter));
}

void FiltersPage::onShow()
{
    return;
}

void FiltersPage::tableCellClicked(const QModelIndex &clicked,
                                   EditableModelView *view_)
{
    // valid column
    if (clicked.column() == 2)
    {
        QMessageBox popup(this->window());

        auto filterText = this->view_->getModel()
                              ->data(clicked.siblingAtColumn(1))
                              .toString();
        auto filterResult = filters::Filter::fromString(filterText);

        if (std::holds_alternative<filters::Filter>(filterResult))
        {
            auto f = std::move(std::get<filters::Filter>(filterResult));
            if (f.returnType() == filters::Type::Bool)
            {
                popup.setIcon(QMessageBox::Icon::Information);
                popup.setWindowTitle("Valid filter");
                popup.setText("Filter is valid");
                popup.setInformativeText(
                    QString("Parsed as:\n%1").arg(f.filterString()));
            }
            else
            {
                popup.setIcon(QMessageBox::Icon::Warning);
                popup.setWindowTitle("Invalid filter");
                popup.setText(QString("Unexpected filter return type"));
                popup.setInformativeText(
                    QString("Expected %1 but got %2")
                        .arg(filters::typeToString(filters::Type::Bool))
                        .arg(filters::typeToString(f.returnType())));
            }
        }
        else
        {
            auto err = std::move(std::get<filters::FilterError>(filterResult));
            popup.setIcon(QMessageBox::Icon::Warning);
            popup.setWindowTitle("Invalid filter");
            popup.setText(QString("Parsing errors occurred:"));
            popup.setInformativeText(err.message);
        }

        popup.exec();
    }
}

bool FiltersPage::filterElements(const QString &query)
{
    std::array fields{0, 1};

    return this->view_->filterSearchResults(query, fields);
}

}  // namespace chatterino
