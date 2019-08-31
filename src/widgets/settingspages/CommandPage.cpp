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
#include "util/CombinePath.hpp"

#include <QLabel>
#include <QTextEdit>

// clang-format off
#define TEXT "{1} => first word &nbsp;&nbsp;&nbsp; {1+} => first word and after &nbsp;&nbsp;&nbsp; {{ => { &nbsp;&nbsp;&nbsp; <a href='https://chatterino.com/help/commands'>more info</a>"
// clang-format on

namespace chatterino {
namespace {
    QString c1settingsPath()
    {
        return combinePath(qgetenv("appdata"),
                           "Chatterino\\Custom\\Commands.txt");
    }
}  // namespace

CommandPage::CommandPage()
    : SettingsPage("Commands", ":/settings/commands.svg")
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
        getApp()->commands->items_.appendItem(
            Command{"/command", "I made a new command HeyGuys"});
    });

    if (QFile(c1settingsPath()).exists())
    {
        auto button = new QPushButton("Import commands from Chatterino 1");
        view->addCustomButton(button);

        QObject::connect(button, &QPushButton::clicked, this, [] {
            QFile c1settings = c1settingsPath();
            c1settings.open(QIODevice::ReadOnly);
            for (auto line : QString(c1settings.readAll())
                                 .split(QRegularExpression("[\r\n]"),
                                        QString::SkipEmptyParts))
            {
                if (int index = line.indexOf(' '); index != -1)
                {
                    getApp()->commands->items_.insertItem(
                        Command(line.mid(0, index), line.mid(index + 1)));
                }
            }
        });
    }

    layout.append(
        this->createCheckBox("Also match the trigger at the end of the message",
                             getSettings()->allowCommandsAtEnd));

    QLabel *text = layout.emplace<QLabel>(TEXT).getElement();
    text->setWordWrap(true);
    text->setStyleSheet("color: #bbb");
    text->setOpenExternalLinks(true);

    // ---- end of layout
    this->commandsEditTimer_.setSingleShot(true);
}

}  // namespace chatterino
