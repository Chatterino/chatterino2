#include "HighlightingPage.hpp"

#include "Application.hpp"
#include "controllers/highlights/HighlightBlacklistModel.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/highlights/HighlightModel.hpp"
#include "controllers/highlights/UserHighlightModel.hpp"
#include "debug/Log.hpp"
#include "singletons/Settings.hpp"
#include "util/LayoutCreator.hpp"
#include "util/StandardItemHelper.hpp"

#include <QColorDialog>
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
#define ALWAYS_PLAY "Play highlight sound even when Chatterino is focused"

namespace chatterino {

HighlightingPage::HighlightingPage()
    : SettingsPage("Highlights", ":/settings/notifications.svg")
{
    auto app = getApp();
    LayoutCreator<HighlightingPage> layoutCreator(this);

    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();
    {
        // GENERAL
        // layout.append(this->createCheckBox(ENABLE_HIGHLIGHTS,
        // getSettings()->enableHighlights));

        // TABS
        auto tabs = layout.emplace<QTabWidget>();
        {
            // HIGHLIGHTS
            auto highlights = tabs.appendTab(new QVBoxLayout, "Messages");
            {
                highlights.emplace<QLabel>("Messages can be highlighted if "
                                           "they match a certain pattern.");

                EditableModelView *view =
                    highlights
                        .emplace<EditableModelView>(
                            app->highlights->createModel(nullptr))
                        .getElement();

                view->addRegexHelpLink();
                view->setTitles({"Pattern", "Flash\ntaskbar", "Play\nsound",
                                 "Enable\nregex", "Case-\nsensitive",
                                 "Custom\nsound", "Color"});
                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    QHeaderView::Fixed);
                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    0, QHeaderView::Stretch);

                // fourtf: make class extrend BaseWidget and add this to
                // dpiChanged
                QTimer::singleShot(1, [view] {
                    view->getTableView()->resizeColumnsToContents();
                    view->getTableView()->setColumnWidth(0, 200);
                });

                view->addButtonPressed.connect([] {
                    getApp()->highlights->phrases.appendItem(HighlightPhrase{
                        "my phrase", true, false, false, false, "",
                        HighlightPhrase::DEFAULT_HIGHLIGHT_COLOR});
                });

                QObject::connect(view->getTableView(), &QTableView::clicked,
                                 [this, view](const QModelIndex &clicked) {
                                     this->tableCellClicked(clicked, view);
                                 });
            }

            auto pingUsers = tabs.appendTab(new QVBoxLayout, "Users");
            {
                pingUsers.emplace<QLabel>(
                    "Messages from a certain user can be highlighted.");
                EditableModelView *view =
                    pingUsers
                        .emplace<EditableModelView>(
                            app->highlights->createUserModel(nullptr))
                        .getElement();

                view->addRegexHelpLink();
                view->getTableView()->horizontalHeader()->hideSection(4);

                // Case-sensitivity doesn't make sense for user names so it is
                // set to "false" by default & the column is hidden
                view->setTitles({"Username", "Flash\ntaskbar", "Play\nsound",
                                 "Enable\nregex", "Case-\nsensitive",
                                 "Custom\nsound", "Color"});
                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    QHeaderView::Fixed);
                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    0, QHeaderView::Stretch);

                // fourtf: make class extrend BaseWidget and add this to
                // dpiChanged
                QTimer::singleShot(1, [view] {
                    view->getTableView()->resizeColumnsToContents();
                    view->getTableView()->setColumnWidth(0, 200);
                });

                view->addButtonPressed.connect([] {
                    getApp()->highlights->highlightedUsers.appendItem(
                        HighlightPhrase{
                            "highlighted user", true, false, false, false, "",
                            HighlightPhrase::DEFAULT_HIGHLIGHT_COLOR});
                });

                QObject::connect(view->getTableView(), &QTableView::clicked,
                                 [this, view](const QModelIndex &clicked) {
                                     this->tableCellClicked(clicked, view);
                                 });
            }

            auto disabledUsers =
                tabs.appendTab(new QVBoxLayout, "Excluded Users");
            {
                disabledUsers.emplace<QLabel>(
                    "This is a list of users (e.g. bots) whose messages should "
                    "<u>not</u> be highlighted.");
                EditableModelView *view =
                    disabledUsers
                        .emplace<EditableModelView>(
                            app->highlights->createBlacklistModel(nullptr))
                        .getElement();

                view->addRegexHelpLink();
                view->setTitles({"Pattern", "Enable\nregex"});
                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    QHeaderView::Fixed);
                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    0, QHeaderView::Stretch);

                // fourtf: make class extrend BaseWidget and add this to
                // dpiChanged
                QTimer::singleShot(1, [view] {
                    view->getTableView()->resizeColumnsToContents();
                    view->getTableView()->setColumnWidth(0, 200);
                });

                view->addButtonPressed.connect([] {
                    getApp()->highlights->blacklistedUsers.appendItem(
                        HighlightBlacklistUser{"blacklisted user", false});
                });
            }
        }

        // MISC
        auto customSound = layout.emplace<QHBoxLayout>().withoutMargin();
        {
            auto fallbackSound = customSound.append(this->createCheckBox(
                "Fallback sound (played when no other sound is set)",
                getSettings()->customHighlightSound));

            auto getSelectFileText = [] {
                const QString value = getSettings()->pathHighlightSound;
                return value.isEmpty() ? "Select custom fallback sound"
                                       : QUrl::fromLocalFile(value).fileName();
            };

            auto selectFile =
                customSound.emplace<QPushButton>(getSelectFileText());

            QObject::connect(
                selectFile.getElement(), &QPushButton::clicked, this,
                [=]() mutable {
                    auto fileName = QFileDialog::getOpenFileName(
                        this, tr("Open Sound"), "",
                        tr("Audio Files (*.mp3 *.wav)"));

                    getSettings()->pathHighlightSound = fileName;
                    selectFile.getElement()->setText(getSelectFileText());

                    // Set check box according to updated value
                    fallbackSound->setCheckState(
                        fileName.isEmpty() ? Qt::Unchecked : Qt::Checked);
                });
        }

        layout.append(createCheckBox(ALWAYS_PLAY,
                                     getSettings()->highlightAlwaysPlaySound));
        layout.append(createCheckBox(
            "Flash taskbar only stops highlighting when chatterino is focused",
            getSettings()->longAlerts));
    }

    // ---- misc
    this->disabledUsersChangedTimer_.setSingleShot(true);
}

void HighlightingPage::tableCellClicked(const QModelIndex &clicked,
                                        EditableModelView *view)
{
    using Column = HighlightModel::Column;

    if (clicked.column() == Column::SoundPath)
    {
        auto fileUrl = QFileDialog::getOpenFileUrl(
            this, tr("Open Sound"), QUrl(), tr("Audio Files (*.mp3 *.wav)"));
        view->getModel()->setData(clicked, fileUrl, Qt::UserRole);
        view->getModel()->setData(clicked, fileUrl.fileName(), Qt::DisplayRole);

        // Enable custom sound check box if user set a sound
        if (!fileUrl.isEmpty())
        {
            QModelIndex checkBox = clicked.siblingAtColumn(Column::PlaySound);
            view->getModel()->setData(checkBox, Qt::Checked,
                                      Qt::CheckStateRole);
        }
    }
    else if (clicked.column() == Column::Color)
    {
        auto initial =
            view->getModel()->data(clicked, Qt::DecorationRole).value<QColor>();
        auto selected = QColorDialog::getColor(
            initial, this, tr("Select Color"), QColorDialog::ShowAlphaChannel);

        // Color is invalid when user clicks "Cancel"
        if (selected.isValid())
            view->getModel()->setData(clicked, selected, Qt::DecorationRole);
    }
}

}  // namespace chatterino
