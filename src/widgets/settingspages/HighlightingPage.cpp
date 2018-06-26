#include "HighlightingPage.hpp"

#include "Application.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/highlights/HighlightModel.hpp"
#include "debug/Log.hpp"
#include "singletons/SettingsManager.hpp"
#include "util/LayoutCreator.hpp"
#include "util/StandardItemHelper.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QFileDialog>
#include <QHeaderView>
#include <QListWidget>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTableView>
#include <QTextEdit>

#define ENABLE_HIGHLIGHTS "Enable Highlighting"
#define HIGHLIGHT_MSG "Highlight messages containing your name"
#define PLAY_SOUND "Play sound when your name is mentioned"
#define FLASH_TASKBAR "Flash taskbar when your name is mentioned"
#define ALWAYS_PLAY "Always play highlight sound (Even if Chatterino is focused)"

namespace chatterino {
namespace widgets {
namespace settingspages {

HighlightingPage::HighlightingPage()
    : SettingsPage("Highlights", ":/images/notifications.svg")
{
    auto app = getApp();
    util::LayoutCreator<HighlightingPage> layoutCreator(this);

    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();
    {
        // GENERAL
        // layout.append(this->createCheckBox(ENABLE_HIGHLIGHTS, app->settings->enableHighlights));

        // TABS
        auto tabs = layout.emplace<QTabWidget>();
        {
            // HIGHLIGHTS
            auto highlights = tabs.appendTab(new QVBoxLayout, "Highlights");
            {
                helper::EditableModelView *view =
                    highlights
                        .emplace<helper::EditableModelView>(app->highlights->createModel(nullptr))
                        .getElement();

                view->setTitles({"Pattern", "Flash taskbar", "Play sound", "Regex"});
                view->getTableView()->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    0, QHeaderView::Stretch);

                // fourtf: make class extrend BaseWidget and add this to dpiChanged
                QTimer::singleShot(1, [view] {
                    view->getTableView()->resizeColumnsToContents();
                    view->getTableView()->setColumnWidth(0, 200);
                });

                view->addButtonPressed.connect([] {
                    getApp()->highlights->phrases.appendItem(
                        controllers::highlights::HighlightPhrase{"my phrase", true, false, false});
                });
            }
            auto disabledUsers = tabs.appendTab(new QVBoxLayout, "Disabled Users");
            {
                auto text = disabledUsers.emplace<QTextEdit>().getElement();

                QObject::connect(text, &QTextEdit::textChanged, this,
                                 [this] { this->disabledUsersChangedTimer.start(200); });

                QObject::connect(
                    &this->disabledUsersChangedTimer, &QTimer::timeout, this, [text, app]() {
                        QStringList list = text->toPlainText().split("\n", QString::SkipEmptyParts);
                        list.removeDuplicates();
                        app->settings->highlightUserBlacklist = list.join("\n") + "\n";
                    });

                app->settings->highlightUserBlacklist.connect([=](const QString &str, auto) {
                    text->setPlainText(str);  //
                });
            }
        }

        // MISC
        auto customSound = layout.emplace<QHBoxLayout>().withoutMargin();
        {
            customSound.append(
                this->createCheckBox("Custom sound", app->settings->customHighlightSound));
            auto selectFile = customSound.emplace<QPushButton>("Select custom sound file");
            QObject::connect(selectFile.getElement(), &QPushButton::clicked, this, [this, app] {
                auto fileName = QFileDialog::getOpenFileName(this, tr("Open Sound"), "",
                                                             tr("Audio Files (*.mp3 *.wav)"));
                app->settings->pathHighlightSound = fileName;
            });
        }

        layout.append(createCheckBox(ALWAYS_PLAY, app->settings->highlightAlwaysPlaySound));
    }

    // ---- misc
    this->disabledUsersChangedTimer.setSingleShot(true);
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
