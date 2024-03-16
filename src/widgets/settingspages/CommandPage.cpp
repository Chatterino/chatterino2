#include "CommandPage.hpp"

#include "Application.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/commands/CommandModel.hpp"
#include "singletons/Settings.hpp"
#include "util/CombinePath.hpp"
#include "util/LayoutCreator.hpp"
#include "util/Qt.hpp"
#include "util/StandardItemHelper.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QColor>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
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
{
    LayoutCreator<CommandPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    this->view = layout
                     .emplace<EditableModelView>(
                         getIApp()->getCommands()->createModel(nullptr))
                     .getElement();

    this->view->setTitles({"Trigger", "Command", "Show In\nMessage Menu"});
    this->view->getTableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);
    // We can safely ignore this signal connection since we own the view
    std::ignore = this->view->addButtonPressed.connect([] {
        getIApp()->getCommands()->items.append(
            Command{"/command", "I made a new command HeyGuys"});
    });

    QObject::connect(view->getModel(), &QAbstractItemModel::rowsInserted, this,
                     [this](const QModelIndex &parent, int first, int last) {
                         this->checkCommandDuplicates();
                     });

    QObject::connect(view->getModel(), &QAbstractItemModel::rowsRemoved, this,
                     [this](const QModelIndex &parent, int first, int last) {
                         this->checkCommandDuplicates();
                     });

    QObject::connect(
        view->getModel(), &QAbstractItemModel::dataChanged, this,
        [this](const QModelIndex &topLeft, const QModelIndex &bottomRight,
               const QVector<int> &roles = QVector<int>()) {
            if (roles.contains(Qt::EditRole))
            {
                this->checkCommandDuplicates();
            }
        });

    // TODO: asyncronously check path
    if (QFile(c1settingsPath()).exists())
    {
        auto *button = new QPushButton("Import commands from Chatterino 1");
        view->addCustomButton(button);

        QObject::connect(button, &QPushButton::clicked, this, [] {
            QFile c1settings = c1settingsPath();
            c1settings.open(QIODevice::ReadOnly);
            for (auto line :
                 QString(c1settings.readAll())
                     .split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts))
            {
                if (int index = line.indexOf(' '); index != -1)
                {
                    getIApp()->getCommands()->items.insert(
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

    this->duplicateCommandWarning =
        layout
            .emplace<QLabel>("Multiple commands with the same trigger found. "
                             "Only one of the commands will work.")
            .getElement();
    this->duplicateCommandWarning->setStyleSheet("color: yellow");
    this->checkCommandDuplicates();

    // ---- end of layout
    this->commandsEditTimer_.setSingleShot(true);
}

bool CommandPage::checkCommandDuplicates()
{
    bool retval = false;
    QMap<QString, QList<int>> map;
    for (int i = 0; i < this->view->getModel()->rowCount(); i++)
    {
        QString commandName =
            this->view->getModel()->index(i, 0).data().toString();
        if (map.contains(commandName))
        {
            QList<int> value = map[commandName];
            value.append(i);
            map.insert(commandName, value);
        }
        else
        {
            map.insert(commandName, {i});
        }
    }

    foreach (const QString &key, map.keys())
    {
        if (map[key].length() != 1)
        {
            retval = true;
            foreach (int value, map[key])
            {
                this->view->getModel()->setData(
                    this->view->getModel()->index(value, 0), QColor("yellow"),
                    Qt::ForegroundRole);
            }
        }
        else
        {
            this->view->getModel()->setData(
                this->view->getModel()->index(map[key][0], 0), QColor("white"),
                Qt::ForegroundRole);
        }
    }

    if (retval)
    {
        this->duplicateCommandWarning->show();
    }
    else
    {
        this->duplicateCommandWarning->hide();
    }

    return retval;
}

}  // namespace chatterino
