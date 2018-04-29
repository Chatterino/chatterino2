#include "commandpage.hpp"

#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextEdit>

#include "application.hpp"
#include "singletons/commandmanager.hpp"
#include "util/layoutcreator.hpp"
#include "util/standarditemhelper.hpp"
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

    QTableView *view = *layout.emplace<QTableView>();

    auto *model = app->commands->createModel(view);
    view->setModel(model);
    model->setHeaderData(0, Qt::Horizontal, "Trigger");
    model->setHeaderData(1, Qt::Horizontal, "Command");
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    view->verticalHeader()->hide();

    auto buttons = layout.emplace<QHBoxLayout>().withoutMargin();
    {
        auto add = buttons.emplace<QPushButton>("Add");
        QObject::connect(*add, &QPushButton::clicked, [model, view] {
            getApp()->commands->items.appendItem(
                singletons::Command{"/command", "I made a new command HeyGuys"});
            view->scrollToBottom();
        });

        auto remove = buttons.emplace<QPushButton>("Remove");
        QObject::connect(*remove, &QPushButton::clicked, [view, model] {
            std::vector<int> indices;

            for (const QModelIndex &index : view->selectionModel()->selectedRows(0)) {
                indices.push_back(index.row());
            }

            std::sort(indices.begin(), indices.end());

            for (int i = indices.size() - 1; i >= 0; i--) {
                model->removeRow(indices[i]);
            }
        });
        buttons->addStretch(1);
    }

    //    QTableView *view = *layout.emplace<QTableView>();
    //    QStandardItemModel *model = new QStandardItemModel(0, 2, view);

    //    view->setModel(model);
    //    model->setHeaderData(0, Qt::Horizontal, "Trigger");
    //    model->setHeaderData(1, Qt::Horizontal, "Command");
    //    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    //    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    //    view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    //    for (const QString &string : app->commands->getCommands()) {
    //        int index = string.indexOf(' ');
    //        if (index == -1) {
    //            model->appendRow({util::stringItem(string), util::stringItem("")});
    //        } else {
    //            model->appendRow(
    //                {util::stringItem(string.mid(0, index)), util::stringItem(string.mid(index +
    //                1))});
    //        }
    //    }

    //    QObject::connect(
    //        model, &QStandardItemModel::dataChanged,
    //        [model](const QModelIndex &topLeft, const QModelIndex &bottomRight,
    //                const QVector<int> &roles) {
    //            QStringList list;

    //            for (int i = 0; i < model->rowCount(); i++) {
    //                QString command = model->item(i, 0)->data(Qt::EditRole).toString();
    //                // int index = command.indexOf(' ');
    //                // if (index != -1) {
    //                //     command = command.mid(index);
    //                // }

    //                list.append(command + " " + model->item(i, 1)->data(Qt::EditRole).toString());
    //            }

    //            getApp()->commands->setCommands(list);
    //        });

    //    auto buttons = layout.emplace<QHBoxLayout>().withoutMargin();
    //    {
    //        auto add = buttons.emplace<QPushButton>("Add");
    //        QObject::connect(*add, &QPushButton::clicked, [model, view] {
    //            model->appendRow({util::stringItem("/command"), util::stringItem("")});
    //            view->scrollToBottom();
    //        });

    //        auto remove = buttons.emplace<QPushButton>("Remove");
    //        QObject::connect(*remove, &QPushButton::clicked, [view, model] {
    //            std::vector<int> indices;

    //            for (const QModelIndex &index : view->selectionModel()->selectedRows(0)) {
    //                indices.push_back(index.row());
    //            }

    //            std::sort(indices.begin(), indices.end());

    //            for (int i = indices.size() - 1; i >= 0; i--) {
    //                model->removeRow(indices[i]);
    //            }
    //        });
    //        buttons->addStretch(1);
    //    }

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
