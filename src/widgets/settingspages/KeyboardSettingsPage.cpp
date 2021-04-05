#include "KeyboardSettingsPage.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "controllers/hotkeys/HotkeyModel.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/dialogs/EditHotkeyDialog.hpp"

#include <QFormLayout>
#include <QLabel>
#include <QTableView>

namespace chatterino {

KeyboardSettingsPage::KeyboardSettingsPage()
{
    auto *app = getApp();

    LayoutCreator<KeyboardSettingsPage> layoutCreator(this);
    auto layout = layoutCreator.emplace<QVBoxLayout>();
    layout.emplace<QLabel>("If a hotkey's configuration is invalid you'll "
                           "receive a popup message when using it.\n"
                           "Some actions of hotkeys don't have any feedback.");

    EditableModelView *view =
        layout.emplace<EditableModelView>(app->hotkeys->createModel(nullptr))
            .getElement();

    view->setTitles({"Name", "Key Combo"});
    view->getTableView()->horizontalHeader()->setVisible(true);
    view->getTableView()->horizontalHeader()->setStretchLastSection(false);
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);

    view->addButtonPressed.connect([] {
        EditHotkeyDialog dialog(nullptr);
        bool wasAccepted = dialog.exec() == 1;

        if (wasAccepted)
        {
            auto newHotkey = dialog.data();
            getApp()->hotkeys->hotkeys_.append(newHotkey);
            getApp()->hotkeys->save();
        }
    });

    QObject::connect(view->getTableView(), &QTableView::doubleClicked,
                     [this, view](const QModelIndex &clicked) {
                         this->tableCellClicked(clicked, view);
                     });
}

void KeyboardSettingsPage::tableCellClicked(const QModelIndex &clicked,
                                            EditableModelView *view)
{
    qDebug() << "table cell clicked!" << clicked.column() << clicked.row();

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
        getApp()->hotkeys->replaceHotkey(hotkey->name(), newHotkey);
        getApp()->hotkeys->save();
    }
}

}  // namespace chatterino
