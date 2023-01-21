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
#include "widgets/TooltipWidget.hpp"

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
    auto app = getApp();

    LayoutCreator<CommandPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    EditableModelView *view =
        layout.emplace<EditableModelView>(app->commands->createModel(nullptr))
            .getElement();

    view->setTitles({"Trigger", "Command", "Show In\nMessage Menu"});
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);
    view->addButtonPressed.connect([] {
        getApp()->commands->items.append(
            Command{"/command", "I made a new command HeyGuys"});
    });

    QItemSelectionModel *selectionModel =
        view->getTableView()->selectionModel();
    QObject::connect(
        selectionModel, &QItemSelectionModel::currentChanged, this,
        [this, view](const QModelIndex &current, const QModelIndex &previous) {
            auto data = previous.sibling(previous.row(), 0).data();
            for (int i = 0; i < current.model()->rowCount(); i++)
            {
                if (i != previous.row() &&
                    data == current.model()->index(i, 0).data())
                {
                    QString warningText = QString("Trigger '%1' already exists")
                                              .arg(data.toString());
                    auto *tooltip = TooltipWidget::instance();
                    tooltip->clearImage();
                    tooltip->setText(warningText);
                    tooltip->adjustSize();
                    auto pos = this->mapToGlobal(
                        QPoint(0, view->getTableView()->rowViewportPosition(
                                      current.row())));
                    tooltip->moveTo(this, pos, false);
                    tooltip->show();
                    break;
                }
            }
        });

    // TODO: asyncronously check path
    if (QFile(c1settingsPath()).exists())
    {
        auto button = new QPushButton("Import commands from Chatterino 1");
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
                    getApp()->commands->items.insert(
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
