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
    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();

    EditableModelView *view = layout
                                  .emplace<EditableModelView>(
                                      app->hotkeys->createModel(nullptr), false)
                                  .getElement();

    view->setTitles({"Name", "Key Combo"});
    view->getTableView()->horizontalHeader()->setVisible(true);
    view->getTableView()->horizontalHeader()->setStretchLastSection(true);

    view->addButtonPressed.connect([] {
        EditHotkeyDialog dialog(std::make_shared<Hotkey>(
            HotkeyScope::Split, QKeySequence("Alt+/"), "showSearch",
            std::vector<QString>(), "Example hotkey"));
        bool wasAccepted = dialog.exec() == 1;

        if (wasAccepted)
        {
            auto newHotkey = dialog.afterEdit();
            getApp()->hotkeys->hotkeys_.append(newHotkey);
            getApp()->hotkeys->save();
        }
        // TODO: display errors
    });
    QTimer::singleShot(1, [view] {
        view->getTableView()->resizeColumnsToContents();
    });

    view->getTableView()->setStyleSheet("background: #333");

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
        auto newHotkey = dialog.afterEdit();
        getApp()->hotkeys->replaceHotkey(hotkey->name(), newHotkey);
        getApp()->hotkeys->save();
    }
    // TODO: display errors
}

}  // namespace chatterino
