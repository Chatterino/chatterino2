#include "widgets/settingspages/KeyboardSettingsPage.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/Hotkey.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "controllers/hotkeys/HotkeyModel.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/dialogs/EditHotkeyDialog.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QTableView>

namespace {

using namespace chatterino;

void tableCellClicked(const QModelIndex &clicked, EditableModelView *view,
                      HotkeyModel *model)
{
    auto hotkey = getIApp()->getHotkeys()->getHotkeyByName(
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
        getIApp()->getHotkeys()->replaceHotkey(hotkey->name(), newHotkey);
        getIApp()->getHotkeys()->save();
    }
}

}  // namespace

namespace chatterino {

KeyboardSettingsPage::KeyboardSettingsPage()
{
    LayoutCreator<KeyboardSettingsPage> layoutCreator(this);
    auto layout = layoutCreator.emplace<QVBoxLayout>();

    auto *model = getIApp()->getHotkeys()->createModel(nullptr);
    EditableModelView *view =
        layout.emplace<EditableModelView>(model).getElement();

    view->setTitles({"Hotkey name", "Keybinding"});
    view->getTableView()->horizontalHeader()->setVisible(true);
    view->getTableView()->horizontalHeader()->setStretchLastSection(false);
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);

    // We can safely ignore this signal connection since we own the view
    std::ignore = view->addButtonPressed.connect([] {
        EditHotkeyDialog dialog(nullptr);
        bool wasAccepted = dialog.exec() == 1;

        if (wasAccepted)
        {
            auto newHotkey = dialog.data();
            getIApp()->getHotkeys()->hotkeys_.append(newHotkey);
            getIApp()->getHotkeys()->save();
        }
    });

    QObject::connect(view->getTableView(), &QTableView::doubleClicked,
                     [view, model](const QModelIndex &clicked) {
                         tableCellClicked(clicked, view, model);
                     });

    auto *resetEverything = new QPushButton("Reset to defaults");
    QObject::connect(resetEverything, &QPushButton::clicked, [this]() {
        auto reply = QMessageBox::question(
            this, "Reset hotkeys",
            "Are you sure you want to reset hotkeys to defaults?",
            QMessageBox::Yes | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes)
        {
            getIApp()->getHotkeys()->resetToDefaults();
        }
    });
    view->addCustomButton(resetEverything);
}

}  // namespace chatterino
