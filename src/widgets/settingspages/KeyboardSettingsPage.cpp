#include "KeyboardSettingsPage.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "controllers/hotkeys/HotkeyModel.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/dialogs/EditHotkeyDialog.hpp"

#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTableView>

namespace chatterino {

KeyboardSettingsPage::KeyboardSettingsPage()
{
    LayoutCreator<KeyboardSettingsPage> layoutCreator(this);
    auto layout = layoutCreator.emplace<QVBoxLayout>();

    auto model = getApp()->hotkeys->createModel(nullptr);
    EditableModelView *view =
        layout.emplace<EditableModelView>(model).getElement();

    view->setTitles({"Hotkey name", "Keybinding"});
    view->getTableView()->horizontalHeader()->setVisible(true);
    view->getTableView()->horizontalHeader()->setStretchLastSection(false);
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);

    view->addButtonPressed.connect([view, model] {
        EditHotkeyDialog dialog(nullptr);
        bool wasAccepted = dialog.exec() == 1;

        if (wasAccepted)
        {
            auto newHotkey = dialog.data();
            int vectorIndex = getApp()->hotkeys->hotkeys_.append(newHotkey);
            getApp()->hotkeys->save();

            // Select and scroll to newly added hotkey
            auto modelRow = model->getModelIndexFromVectorIndex(vectorIndex);
            auto modelIndex = model->index(modelRow, 0);
            view->selectRow(modelRow);
            view->getTableView()->scrollTo(modelIndex,
                                           QAbstractItemView::PositionAtCenter);
        }
    });

    QObject::connect(view->getTableView(), &QTableView::doubleClicked,
                     [this, view, model](const QModelIndex &clicked) {
                         this->tableCellClicked(clicked, view, model);
                     });

    QPushButton *resetEverything = new QPushButton("Reset to defaults");
    QObject::connect(resetEverything, &QPushButton::clicked, [this]() {
        auto reply = QMessageBox::question(
            this, "Reset hotkeys",
            "Are you sure you want to reset hotkeys to defaults?",
            QMessageBox::Yes | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes)
        {
            getApp()->hotkeys->resetToDefaults();
        }
    });
    view->addCustomButton(resetEverything);
}

void KeyboardSettingsPage::tableCellClicked(const QModelIndex &clicked,
                                            EditableModelView *view,
                                            HotkeyModel *model)
{
    auto hotkey = getApp()->hotkeys->getHotkeyByName(
        clicked.siblingAtColumn(0).data(Qt::EditRole).toString());
    if (!hotkey)
    {
        return;  // clicked on header or invalid hotkey
    }
    EditHotkeyDialog dialog(hotkey);
    bool wasAccepted = dialog.exec() == 1;

    if (wasAccepted)
    {
        auto newHotkey = dialog.data();
        auto vectorIndex =
            getApp()->hotkeys->replaceHotkey(hotkey->name(), newHotkey);
        getApp()->hotkeys->save();

        // Select the replaced hotkey
        auto modelRow = model->getModelIndexFromVectorIndex(vectorIndex);
        auto modelIndex = model->index(modelRow, 0);
        view->selectRow(modelRow);
    }
}

}  // namespace chatterino
