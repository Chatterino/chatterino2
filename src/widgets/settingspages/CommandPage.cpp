#include "CommandPage.hpp"

#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextEdit>

#include "Application.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/commands/CommandModel.hpp"
#include "util/LayoutCreator.hpp"
#include "util/StandardItemHelper.hpp"
#include "widgets/helper/EditableModelView.hpp"
//#include "widgets/helper/ComboBoxItemDelegate.hpp"

#include <QLabel>
#include <QTextEdit>

// clang-format off
#define TEXT "{1} => first word, {2} => second word, ...\n"\
    "{1+} => first word and after, {2+} => second word and after, ...\n"\
    "{{1} => {1}"
// clang-format on

namespace chatterino {

CommandPage::CommandPage()
    : SettingsPage("Commands", ":/images/commands.svg")
{
    auto app = getApp();

    LayoutCreator<CommandPage> layoutCreator(this);
    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();

    EditableModelView *view =
        layout.emplace<EditableModelView>(app->commands->createModel(nullptr))
            .getElement();

    view->setTitles({"Trigger", "Command"});
    view->getTableView()->horizontalHeader()->setStretchLastSection(true);
    view->addButtonPressed.connect([] {
        getApp()->commands->items.appendItem(
            Command{"/command", "I made a new command HeyGuys"});
    });

    layout.append(
        this->createCheckBox("Also match the trigger at the end of the message",
                             app->settings->allowCommandsAtEnd));

    QLabel *text = layout.emplace<QLabel>(TEXT).getElement();
    text->setWordWrap(true);
    text->setStyleSheet("color: #bbb");

    // ---- end of layout
    this->commandsEditTimer_.setSingleShot(true);
}

}  // namespace chatterino
