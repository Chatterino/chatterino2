#include "widgets/settingspages/CommandPage.hpp"

#include "Application.hpp"
#include "common/Literals.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/commands/CommandModel.hpp"
#include "singletons/Settings.hpp"
#include "util/CombinePath.hpp"
#include "util/LayoutCreator.hpp"
#include "util/StandardItemHelper.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QColor>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextEdit>

namespace {

using namespace chatterino;
using namespace literals;

// clang-format off
inline const QString HELP_TEXT = u"{1} => first word &nbsp;&nbsp;&nbsp; {1+} => first word and after &nbsp;&nbsp;&nbsp; {{ => { &nbsp;&nbsp;&nbsp; <a href='https://chatterino.com/help/commands'>more info</a>"_s;
// clang-format on

QString c1settingsPath()
{
    return combinePath(qgetenv("appdata"), "Chatterino\\Custom\\Commands.txt");
}

void checkCommandDuplicates(EditableModelView *view, QLabel *duplicateWarning)
{
    bool foundDuplicateTrigger = false;

    // Maps command triggers to model row indices
    std::unordered_map<QString, std::vector<int>> commands;

    for (int i = 0; i < view->getModel()->rowCount(); i++)
    {
        QString commandTrigger =
            view->getModel()->index(i, 0).data().toString();
        commands[commandTrigger].push_back(i);
    }

    for (const auto &[commandTrigger, rowIndices] : commands)
    {
        assert(!rowIndices.empty());

        if (rowIndices.size() > 1)
        {
            foundDuplicateTrigger = true;

            for (const auto &rowIndex : rowIndices)
            {
                view->getModel()->setData(view->getModel()->index(rowIndex, 0),
                                          QColor("yellow"), Qt::ForegroundRole);
            }
        }
        else
        {
            view->getModel()->setData(view->getModel()->index(rowIndices[0], 0),
                                      QColor("white"), Qt::ForegroundRole);
        }
    }

    if (foundDuplicateTrigger)
    {
        duplicateWarning->show();
    }
    else
    {
        duplicateWarning->hide();
    }
}

}  // namespace

namespace chatterino {

CommandPage::CommandPage()
{
    LayoutCreator<CommandPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    auto *view = layout
                     .emplace<EditableModelView>(
                         getApp()->getCommands()->createModel(nullptr))
                     .getElement();

    view->setTitles({"Trigger", "Command", "Show In\nMessage Menu"});
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);
    // We can safely ignore this signal connection since we own the view
    std::ignore = view->addButtonPressed.connect([] {
        getApp()->getCommands()->items.append(
            Command{"/command", "I made a new command HeyGuys"});
    });

    // TODO: asyncronously check path
    if (QFile(c1settingsPath()).exists())
    {
        auto *button = new QPushButton("Import commands from Chatterino 1");
        view->addCustomButton(button);

        QObject::connect(button, &QPushButton::clicked, this, [] {
            QFile c1settings(c1settingsPath());
            c1settings.open(QIODevice::ReadOnly);
            for (auto line :
                 QString(c1settings.readAll())
                     .split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts))
            {
                if (int index = line.indexOf(' '); index != -1)
                {
                    getApp()->getCommands()->items.insert(
                        Command(line.mid(0, index), line.mid(index + 1)));
                }
            }
        });
    }

    QLabel *text = layout.emplace<QLabel>(HELP_TEXT).getElement();
    text->setWordWrap(true);
    text->setStyleSheet("color: #bbb");
    text->setOpenExternalLinks(true);

    auto *duplicateWarning =
        layout
            .emplace<QLabel>("Multiple commands with the same trigger found. "
                             "Only one of the commands will work.")
            .getElement();
    duplicateWarning->setStyleSheet("color: yellow");

    // NOTE: These signals mean that the duplicate check happens in the middle of a row being moved, where he index can be wrong.
    // This should be reconsidered, or potentially changed in the signalvectormodel. Or maybe we rely on a SignalVectorModel signal instead
    QObject::connect(view->getModel(), &QAbstractItemModel::rowsInserted, this,
                     [view, duplicateWarning]() {
                         checkCommandDuplicates(view, duplicateWarning);
                     });

    QObject::connect(view->getModel(), &QAbstractItemModel::rowsRemoved, this,
                     [view, duplicateWarning]() {
                         checkCommandDuplicates(view, duplicateWarning);
                     });

    QObject::connect(view->getModel(), &QAbstractItemModel::dataChanged, this,
                     [view, duplicateWarning](const QModelIndex &topLeft,
                                              const QModelIndex &bottomRight,
                                              const QVector<int> &roles) {
                         (void)topLeft;
                         (void)bottomRight;
                         if (roles.contains(Qt::EditRole))
                         {
                             checkCommandDuplicates(view, duplicateWarning);
                         }
                     });

    checkCommandDuplicates(view, duplicateWarning);

    // ---- end of layout
    this->commandsEditTimer_.setSingleShot(true);
}

}  // namespace chatterino
