// SPDX-FileCopyrightText: 2021 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/settingspages/NicknamesPage.hpp"

#include "Application.hpp"
#include "controllers/nicknames/Nickname.hpp"
#include "controllers/nicknames/NicknamesModel.hpp"
#include "singletons/Settings.hpp"
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
    layout.emplace<QLabel>(
        "Twitch account entries match by User ID. Legacy username and regex "
        "entries match by name."
        "\nTwitch account entries cannot be reordered.");

    auto *model = new NicknamesModel(getApp()->getUserData(), nullptr);
    model->initialize(&getSettings()->nicknames);
    auto *view = layout.emplace<EditableModelView>(model).getElement();
    this->view_ = view;

    view->setTitles({"Match on", "Username / pattern", "User ID", "Nickname",
                     "Case-sensitive"});
    view->getTableView()->setDragDropMode(QAbstractItemView::NoDragDrop);
    auto *header = view->getTableView()->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::Stretch);
    QObject::connect(model, &QAbstractItemModel::rowsInserted, view,
                     [table = view->getTableView()] {
                         // ResizeToContents can retain widths calculated for
                         // the empty table, so recalculate the content-sized
                         // columns after rows are inserted.
                         QTimer::singleShot(0, table, [table] {
                             table->resizeColumnToContents(0);
                             table->resizeColumnToContents(2);
                             table->resizeColumnToContents(4);
                         });
                     });
    view->addRegexHelpLink();

    // We can safely ignore this signal connection since we own the view
    std::ignore = view->addButtonPressed.connect([] {
        getSettings()->nicknames.append(
            Nickname{"Username", "Nickname", false, false});
    });
}

bool NicknamesPage::filterElements(const QString &query)
{
    std::array fields{0, 1, 2, 3};
    return this->view_->filterSearchResults(query, fields);
}

}  // namespace chatterino
