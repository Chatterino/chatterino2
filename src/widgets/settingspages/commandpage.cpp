#include "commandpage.hpp"

#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextEdit>

#include "application.hpp"
#include "controllers/commands/commandcontroller.hpp"
#include "controllers/commands/commandmodel.hpp"
#include "util/layoutcreator.hpp"
#include "util/standarditemhelper.hpp"
#include "widgets/helper/editablemodelview.hpp"
//#include "widgets/helper/comboboxitemdelegate.hpp"

#include <QLabel>
#include <QTextEdit>

// clang-format off
#define TEXT "{1} => first word, {2} => second word, ...\n"\
    "{1+} => first word and after, {2+} => second word and after, ...\n"\
    "{{1} => {1}"
// clang-format on

namespace chatterino {
namespace widgets {
namespace settingspages {

CommandPage::CommandPage()
    : SettingsPage("Commands", ":/images/commands.svg")
{
    auto app = getApp();

    util::LayoutCreator<CommandPage> layoutCreator(this);
    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();

    helper::EditableModelView *view =
        *layout.emplace<helper::EditableModelView>(app->commands->createModel(nullptr));

    view->setTitles({"Trigger", "Command"});
    view->getTableView()->horizontalHeader()->setStretchLastSection(true);
    view->addButtonPressed.connect([] {
        getApp()->commands->items.appendItem(
            controllers::commands::Command{"/command", "I made a new command HeyGuys"});
    });

    layout.append(this->createCheckBox("Also match the trigger at the end of the message",
                                       app->settings->allowCommandsAtEnd));

    QLabel *text = *layout.emplace<QLabel>(TEXT);
    text->setWordWrap(true);
    text->setStyleSheet("color: #bbb");

    // ---- end of layout
    this->commandsEditTimer.setSingleShot(true);
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
