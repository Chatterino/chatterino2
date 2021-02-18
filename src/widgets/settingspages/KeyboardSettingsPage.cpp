#include "KeyboardSettingsPage.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "controllers/hotkeys/HotkeyModel.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"

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

    view->setTitles({"Name", "Key Combo", "Action", "Arguments"});
    view->getTableView()->horizontalHeader()->setVisible(true);
    view->getTableView()->horizontalHeader()->setStretchLastSection(true);

    view->addButtonPressed.connect([] {
        qCDebug(chatterinoHotkeys) << "xd";
    });
    QTimer::singleShot(1, [view] {
        view->getTableView()->resizeColumnsToContents();
        view->getTableView()->setColumnWidth(0, 175);
        view->getTableView()->setColumnWidth(1, 80);
        view->getTableView()->setColumnWidth(2, 100);
    });

    view->getTableView()->setStyleSheet("background: #333");
}

}  // namespace chatterino
