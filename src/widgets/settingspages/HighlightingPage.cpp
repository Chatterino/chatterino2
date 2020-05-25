#include "HighlightingPage.hpp"

#include "Application.hpp"
#include "controllers/highlights/BadgeHighlightModel.hpp"
#include "controllers/highlights/HighlightBlacklistModel.hpp"
#include "controllers/highlights/HighlightModel.hpp"
#include "controllers/highlights/UserHighlightModel.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/LayoutCreator.hpp"
#include "util/StandardItemHelper.hpp"
#include "widgets/dialogs/BadgePickerDialog.hpp"
#include "widgets/dialogs/ColorPickerDialog.hpp"

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

namespace {
    QList<DisplayBadge> availableBadges = {
        {"Broadcaster", "broadcaster", "1"},
        {"Admin", "admin", "1"},
        {"Staff", "staff", "1"},
        {"Global Moderator", "global_mod", "1"},
        {"Moderator", "moderator", "1"},
        {"Verified", "partner", "1"},
        {"VIP", "vip", "1"},
    };
}

HighlightingPage::HighlightingPage()
{
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
                highlights.emplace<QLabel>(
                    "Play notification sounds and highlight messages based on "
                    "certain patterns.");

                auto view =
                    highlights
                        .emplace<EditableModelView>(
                            (new HighlightModel(nullptr))
                                ->initialized(
                                    &getSettings()->highlightedMessages))
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
                    getSettings()->highlightedMessages.append(HighlightPhrase{
                        "my phrase", true, false, false, false, "",
                        *ColorProvider::instance().color(
                            ColorType::SelfHighlight)});
                });

                QObject::connect(view->getTableView(), &QTableView::clicked,
                                 [this, view](const QModelIndex &clicked) {
                                     this->tableCellClicked(clicked, view);
                                 });
            }

            auto pingUsers = tabs.appendTab(new QVBoxLayout, "Users");
            {
                pingUsers.emplace<QLabel>(
                    "Play notification sounds and highlight messages from "
                    "certain users.\n"
                    "User highlights are prioritized over message "
                    "highlights.");
                EditableModelView *view =
                    pingUsers
                        .emplace<EditableModelView>(
                            (new UserHighlightModel(nullptr))
                                ->initialized(&getSettings()->highlightedUsers))
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
                    getSettings()->highlightedUsers.append(HighlightPhrase{
                        "highlighted user", true, false, false, false, "",
                        *ColorProvider::instance().color(
                            ColorType::SelfHighlight)});
                });

                QObject::connect(view->getTableView(), &QTableView::clicked,
                                 [this, view](const QModelIndex &clicked) {
                                     this->tableCellClicked(clicked, view);
                                 });
            }

            auto badgeHighlights = tabs.appendTab(new QVBoxLayout, "Badges");
            {
                badgeHighlights.emplace<QLabel>(
                    "Play notification sounds and highlight messages based on "
                    "user badges.");
                auto view = badgeHighlights
                                .emplace<EditableModelView>(
                                    (new BadgeHighlightModel(nullptr))
                                        ->initialized(
                                            &getSettings()->highlightedBadges))
                                .getElement();
                view->setTitles({"Name", "Flash\ntaskbar", "Play\nsound",
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

                view->addButtonPressed.connect([this] {
                    auto d = std::make_shared<BadgePickerDialog>(
                        availableBadges, this);

                    d->setWindowTitle("Choose badge");
                    if (d->exec() == QDialog::Accepted)
                    {
                        auto s = d->getSelection();
                        getSettings()->highlightedBadges.append(
                            HighlightBadge{s->badgeName(), s->badgeVersion(),
                                           s->displayName(), false, false, "",
                                           ColorProvider::instance().color(
                                               ColorType::SelfHighlight)});
                    }
                });

                QObject::connect(view->getTableView(), &QTableView::clicked,
                                 [this, view](const QModelIndex &clicked) {
                                     this->tableCellClicked(clicked, view);
                                 });
            }

            auto disabledUsers =
                tabs.appendTab(new QVBoxLayout, "Blacklisted Users");
            {
                disabledUsers.emplace<QLabel>(
                    "Disable notification sounds and highlights from certain "
                    "users (e.g. bots).");
                EditableModelView *view =
                    disabledUsers
                        .emplace<EditableModelView>(
                            (new HighlightBlacklistModel(nullptr))
                                ->initialized(&getSettings()->blacklistedUsers))
                        .getElement();

                view->addRegexHelpLink();
                view->setTitles({"Username", "Enable\nregex"});
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
                    getSettings()->blacklistedUsers.append(
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
}  // namespace chatterino

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
    else if (clicked.column() == Column::Color &&
             clicked.row() != HighlightModel::WHISPER_ROW)
    {
        auto initial =
            view->getModel()->data(clicked, Qt::DecorationRole).value<QColor>();

        auto dialog = new ColorPickerDialog(initial, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
        dialog->closed.connect([=] {
            QColor selected = dialog->selectedColor();

            if (selected.isValid())
            {
                view->getModel()->setData(clicked, selected,
                                          Qt::DecorationRole);

                // Hacky (?) way to figure out what tab the cell was clicked in
                const bool fromMessages = (dynamic_cast<HighlightModel *>(
                                               view->getModel()) != nullptr);

                if (fromMessages)
                {
                    /*
                     * For preset highlights in the "Messages" tab, we need to
                     * manually update the color map.
                     */
                    auto instance = ColorProvider::instance();
                    switch (clicked.row())
                    {
                        case 0:
                            instance.updateColor(ColorType::SelfHighlight,
                                                 selected);
                            break;
                        case 1:
                            instance.updateColor(ColorType::Whisper, selected);
                            break;
                        case 2:
                            instance.updateColor(ColorType::Subscription,
                                                 selected);
                            break;
                    }
                }
            }
        });
    }
}

}  // namespace chatterino
