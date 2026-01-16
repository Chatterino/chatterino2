// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/dialogs/CustomSearchEnginesDialog.hpp"

#include "common/CustomSearchEngine.hpp"
#include "singletons/Settings.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"
#include "widgets/settingspages/CustomSearchEngineModel.hpp"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QTableView>
#include <QVBoxLayout>

namespace chatterino {

CustomSearchEnginesDialog::CustomSearchEnginesDialog(QWidget *parent)
    : QDialog(parent)
{
    this->setWindowTitle("Custom Search Engines");
    this->resize(600, 400);

    LayoutCreator<CustomSearchEnginesDialog> layoutCreator(this);
    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();

    auto *view = layout
                     .emplace<EditableModelView>(
                         (new CustomSearchEngineModel(nullptr))
                             ->initialized(&getSettings()->customSearchEngines),
                         false)
                     .getElement();

    view->setTitles({"Name", "URL"});
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);

    std::ignore = view->addButtonPressed.connect([] {
        getSettings()->customSearchEngines.append(
            CustomSearchEngine{"", "https://example.com/?q="});
    });

    auto *buttonBox =
        layout.emplace<QDialogButtonBox>(QDialogButtonBox::Ok).getElement();
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this,
                     &QDialog::accept);
}

}  // namespace chatterino
