#include "widgets/settingspages/NicknamesPage.hpp"

#include "controllers/nicknames/Nickname.hpp"
#include "controllers/nicknames/NicknamesModel.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QHeaderView>
#include <QTableView>

namespace chatterino {

NicknamesPage::NicknamesPage()
{
    LayoutCreator<NicknamesPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    layout.emplace<QLabel>(
        "Nicknames do not work with features such as user highlights and "
        "filters."
        "\nWith those features you will still need to use the user's original "
        "name.");
    this->view_ = layout
                      .emplace<EditableModelView>(
                          (new NicknamesModel(nullptr))
                              ->initialized(&getSettings()->nicknames))
                      .getElement();

    this->view_->setTitles(
        {"Username", "Nickname", "Enable regex", "Case-sensitive"});
    this->view_->getTableView()->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Fixed);
    this->view_->getTableView()->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    this->view_->getTableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);

    // We can safely ignore this signal connection since we own the this->view_
    std::ignore = this->view_->addButtonPressed.connect([] {
        getSettings()->nicknames.append(
            Nickname{"Username", "Nickname", false, false});
    });

    QTimer::singleShot(1, [this] {
        this->view_->getTableView()->resizeColumnsToContents();
        this->view_->getTableView()->setColumnWidth(0, 200);
    });
}

bool NicknamesPage::filterElements(const QString &query)
{
    std::array fields{0, 1};

    return this->view_->filterSearchResults(query, fields);
}

}  // namespace chatterino
