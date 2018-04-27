#include "commandpage.hpp"

#include "application.hpp"
#include "singletons/commandmanager.hpp"
#include "util/layoutcreator.hpp"

#include <QLabel>
#include <QTextEdit>

// clang-format off
#define TEXT "One command per line.\n"\
    "\"/cmd example command\" will print \"example command\" when you type /cmd in chat.\n"\
    "{1} will be replaced with the first word you type after then command, {2} with the second and so on.\n"\
    "{1+} will be replaced with first word and everything after, {2+} with everything after the second word and so on\n"\
    "Duplicate commands will be ignored."
// clang-format on

namespace chatterino {
namespace widgets {
namespace settingspages {

CommandPage::CommandPage()
    : SettingsPage("Commands", ":/images/commands.svg")
{
    util::LayoutCreator<CommandPage> layoutCreator(this);
    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();

    layout.emplace<QLabel>(TEXT)->setWordWrap(true);

    layout.append(this->getCommandsTextEdit());

    // ---- end of layout
    this->commandsEditTimer.setSingleShot(true);
}

QTextEdit *CommandPage::getCommandsTextEdit()
{
    auto app = getApp();

    // cancel
    QStringList currentCommands = app->commands->getCommands();

    this->onCancel.connect(
        [currentCommands, app] { app->commands->setCommands(currentCommands); });

    // create text edit
    QTextEdit *textEdit = new QTextEdit;

    textEdit->setPlainText(QString(app->commands->getCommands().join('\n')));

    QObject::connect(textEdit, &QTextEdit::textChanged,
                     [this] { this->commandsEditTimer.start(200); });

    QObject::connect(&this->commandsEditTimer, &QTimer::timeout, [textEdit, app] {
        QString text = textEdit->toPlainText();
        QStringList lines = text.split(QRegularExpression("(\r?\n|\r\n?)"));

        app->commands->setCommands(lines);
    });

    return textEdit;
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
