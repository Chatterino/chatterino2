#include "widgets/settingspages/HighlightingPage.hpp"

#include "Application.hpp"
#include "controllers/highlights/BadgeHighlightModel.hpp"
#include "controllers/highlights/HighlightBadge.hpp"
#include "controllers/highlights/HighlightBlacklistModel.hpp"
#include "controllers/highlights/HighlightBlacklistUser.hpp"
#include "controllers/highlights/HighlightModel.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "controllers/highlights/UserHighlightModel.hpp"
#include "providers/colors/ColorProvider.hpp"
#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/dialogs/BadgePickerDialog.hpp"
#include "widgets/dialogs/ColorPickerDialog.hpp"
#include "widgets/helper/color/ColorItemDelegate.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QFileDialog>
#include <QHeaderView>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QTabWidget>

namespace chatterino {

namespace {
    // Add additional badges for highlights here
    QList<DisplayBadge> availableBadges = {
        {"Broadcaster", "broadcaster"},
        {"Admin", "admin"},
        {"Staff", "staff"},
        {"Moderator", "moderator"},
        {"Verified", "partner"},
        {"VIP", "vip"},
        {"Founder", "founder"},
        {"Subscriber", "subscriber"},
        {"Predicted Blue", "predictions/blue-1,predictions/blue-2"},
        {"Predicted Pink", "predictions/pink-2,predictions/pink-1"},
    };
}  // namespace

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
        this->tabWidget_ = &*tabs;
        {
            // HIGHLIGHTS
            auto highlights = tabs.appendTab(new QVBoxLayout, "Messages");
            {
                highlights.emplace<QLabel>(
                    "Play notification sounds and highlight messages based on "
                    "certain patterns.\n"
                    "Message highlights are prioritized over badge highlights "
                    "and user highlights.");

                this->viewMessages_ =
                    highlights
                        .emplace<EditableModelView>(
                            (new HighlightModel(nullptr))
                                ->initialized(
                                    &getSettings()->highlightedMessages))
                        .getElement();
                this->viewMessages_->addRegexHelpLink();
                this->viewMessages_->setTitles(
                    {"Pattern", "Show in\nMentions", "Flash\ntaskbar",
                     "Enable\nregex", "Case-\nsensitive", "Play\nsound",
                     "Custom\nsound", "Color"});
                this->viewMessages_->getTableView()
                    ->horizontalHeader()
                    ->setSectionResizeMode(QHeaderView::Fixed);
                this->viewMessages_->getTableView()
                    ->horizontalHeader()
                    ->setSectionResizeMode(0, QHeaderView::Stretch);
                this->viewMessages_->getTableView()->setItemDelegateForColumn(
                    HighlightModel::Column::Color,
                    new ColorItemDelegate(viewMessages_));

                // fourtf: make class extrend BaseWidget and add this to
                // dpiChanged
                QTimer::singleShot(1, [this] {
                    this->viewMessages_->getTableView()
                        ->resizeColumnsToContents();
                    this->viewMessages_->getTableView()->setColumnWidth(0, 400);
                });

                // We can safely ignore this signal connection since we own the this->viewMessages_
                std::ignore = this->viewMessages_->addButtonPressed.connect([] {
                    getSettings()->highlightedMessages.append(HighlightPhrase{
                        "my phrase", true, true, false, false, false, "",
                        *ColorProvider::instance().color(
                            ColorType::SelfHighlight)});
                });

                QObject::connect(
                    this->viewMessages_->getTableView(), &QTableView::clicked,
                    [this](const QModelIndex &clicked) {
                        this->tableCellClicked(clicked, this->viewMessages_,
                                               HighlightTab::Messages);
                    });
            }

            auto pingUsers = tabs.appendTab(new QVBoxLayout, "Users");
            {
                pingUsers.emplace<QLabel>(
                    "Play notification sounds and highlight messages from "
                    "certain users.\n"
                    "User highlights are prioritized over badge highlights, "
                    "but under message highlights.");
                this->viewUsers_ =
                    pingUsers
                        .emplace<EditableModelView>(
                            (new UserHighlightModel(nullptr))
                                ->initialized(&getSettings()->highlightedUsers))
                        .getElement();

                this->viewUsers_->addRegexHelpLink();
                this->viewUsers_->getTableView()
                    ->horizontalHeader()
                    ->hideSection(HighlightModel::Column::UseRegex);
                this->viewUsers_->getTableView()
                    ->horizontalHeader()
                    ->hideSection(HighlightModel::Column::CaseSensitive);
                // Case-sensitivity doesn't make sense for user names so it is
                // set to "false" by default & the column is hidden
                this->viewUsers_->setTitles({"Username", "Show in\nMentions",
                                             "Flash\ntaskbar", "Enable\nregex",
                                             "Case-\nsensitive", "Play\nsound",
                                             "Custom\nsound", "Color"});
                this->viewUsers_->getTableView()
                    ->horizontalHeader()
                    ->setSectionResizeMode(QHeaderView::Fixed);
                this->viewUsers_->getTableView()
                    ->horizontalHeader()
                    ->setSectionResizeMode(0, QHeaderView::Stretch);
                this->viewUsers_->getTableView()->setItemDelegateForColumn(
                    UserHighlightModel::Column::Color,
                    new ColorItemDelegate(viewUsers_));

                // fourtf: make class extrend BaseWidget and add this to
                // dpiChanged
                QTimer::singleShot(1, [this] {
                    this->viewUsers_->getTableView()->resizeColumnsToContents();
                    this->viewUsers_->getTableView()->setColumnWidth(0, 200);
                });

                // We can safely ignore this signal connection since we own the this->viewUsers_
                std::ignore = this->viewUsers_->addButtonPressed.connect([] {
                    getSettings()->highlightedUsers.append(HighlightPhrase{
                        "highlighted user", true, true, false, false, false, "",
                        *ColorProvider::instance().color(
                            ColorType::SelfHighlight)});
                });

                QObject::connect(
                    this->viewUsers_->getTableView(), &QTableView::clicked,
                    [this](const QModelIndex &clicked) {
                        this->tableCellClicked(clicked, this->viewUsers_,
                                               HighlightTab::Users);
                    });
            }

            auto badgeHighlights = tabs.appendTab(new QVBoxLayout, "Badges");
            {
                badgeHighlights.emplace<QLabel>(
                    "Play notification sounds and highlight messages based on "
                    "user badges.\n"
                    "Badge highlights are prioritzed under user and message "
                    "highlights.");
                this->viewBadges_ =
                    badgeHighlights
                        .emplace<EditableModelView>(
                            (new BadgeHighlightModel(nullptr))
                                ->initialized(
                                    &getSettings()->highlightedBadges))
                        .getElement();
                this->viewBadges_->setTitles({"Name", "Show In\nMentions",
                                              "Flash\ntaskbar", "Play\nsound",
                                              "Custom\nsound", "Color"});
                this->viewBadges_->getTableView()
                    ->horizontalHeader()
                    ->setSectionResizeMode(QHeaderView::Fixed);
                this->viewBadges_->getTableView()
                    ->horizontalHeader()
                    ->setSectionResizeMode(0, QHeaderView::Stretch);
                this->viewBadges_->getTableView()->setItemDelegateForColumn(
                    BadgeHighlightModel::Column::Color,
                    new ColorItemDelegate(viewBadges_));

                // fourtf: make class extrend BaseWidget and add this to
                // dpiChanged
                QTimer::singleShot(1, [this] {
                    this->viewBadges_->getTableView()
                        ->resizeColumnsToContents();
                    this->viewBadges_->getTableView()->setColumnWidth(0, 200);
                });

                // We can safely ignore this signal connection since we own the this->viewBadges_
                std::ignore =
                    this->viewBadges_->addButtonPressed.connect([this] {
                        auto d = std::make_shared<BadgePickerDialog>(
                            availableBadges, this);

                        d->setWindowTitle("Choose badge");
                        if (d->exec() == QDialog::Accepted)
                        {
                            auto s = d->getSelection();
                            if (!s)
                            {
                                return;
                            }
                            getSettings()->highlightedBadges.append(
                                HighlightBadge{s->badgeName(), s->displayName(),
                                               false, false, false, "",
                                               *ColorProvider::instance().color(
                                                   ColorType::SelfHighlight)});
                        }
                    });

                QObject::connect(
                    this->viewBadges_->getTableView(), &QTableView::clicked,
                    [this](const QModelIndex &clicked) {
                        this->tableCellClicked(clicked, this->viewBadges_,
                                               HighlightTab::Badges);
                    });
            }

            auto disabledUsers =
                tabs.appendTab(new QVBoxLayout, "Blacklisted Users");
            {
                disabledUsers.emplace<QLabel>(
                    "Disable notification sounds and highlights from certain "
                    "users (e.g. bots).");
                this->viewBlacklistedUsers_ =
                    disabledUsers
                        .emplace<EditableModelView>(
                            (new HighlightBlacklistModel(nullptr))
                                ->initialized(&getSettings()->blacklistedUsers))
                        .getElement();

                this->viewBlacklistedUsers_->addRegexHelpLink();
                this->viewBlacklistedUsers_->setTitles(
                    {"Username", "Enable\nregex"});
                this->viewBlacklistedUsers_->getTableView()
                    ->horizontalHeader()
                    ->setSectionResizeMode(QHeaderView::Fixed);
                this->viewBlacklistedUsers_->getTableView()
                    ->horizontalHeader()
                    ->setSectionResizeMode(0, QHeaderView::Stretch);

                // fourtf: make class extrend BaseWidget and add this to
                // dpiChanged
                QTimer::singleShot(1, [this] {
                    this->viewBlacklistedUsers_->getTableView()
                        ->resizeColumnsToContents();
                    this->viewBlacklistedUsers_->getTableView()->setColumnWidth(
                        0, 200);
                });

                // We can safely ignore this signal connection since we own the this->viewBlacklistedUsers_
                std::ignore =
                    this->viewBlacklistedUsers_->addButtonPressed.connect([] {
                        getSettings()->blacklistedUsers.append(
                            HighlightBlacklistUser{"blacklisted user", false});
                    });
            }
        }

        // MISC
        auto customSound = layout.emplace<QHBoxLayout>().withoutMargin();
        {
            auto label = customSound.append(this->createLabel<QString>(
                [](const auto &value) {
                    if (value.isEmpty())
                    {
                        return QString("Default sound: Chatterino Ping");
                    }

                    auto url = QUrl::fromLocalFile(value);
                    return QString("Default sound: <a href=\"%1\"><span "
                                   "style=\"color: white\">%2</span></a>")
                        .arg(url.toString(QUrl::FullyEncoded),
                             shortenString(url.fileName(), 50));
                },
                getSettings()->pathHighlightSound));
            label->setToolTip(
                "This sound will play for all highlight phrases that have "
                "sound enabled and don't have a custom sound set.");
            label->setTextFormat(Qt::RichText);
            label->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                           Qt::LinksAccessibleByKeyboard);
            label->setOpenExternalLinks(true);
            customSound->setStretchFactor(label.getElement(), 1);

            auto clearSound = customSound.emplace<QPushButton>("Clear");
            auto selectFile = customSound.emplace<QPushButton>("Change...");

            QObject::connect(selectFile.getElement(), &QPushButton::clicked,
                             this, [this]() mutable {
                                 auto fileName = QFileDialog::getOpenFileName(
                                     this, tr("Open Sound"), "",
                                     tr("Audio Files (*.mp3 *.wav)"));

                                 getSettings()->pathHighlightSound = fileName;
                             });
            QObject::connect(clearSound.getElement(), &QPushButton::clicked,
                             this, [=]() mutable {
                                 getSettings()->pathHighlightSound = QString();
                             });

            getSettings()->pathHighlightSound.connect(
                [clearSound = clearSound.getElement()](const auto &value) {
                    if (value.isEmpty())
                    {
                        clearSound->hide();
                    }
                    else
                    {
                        clearSound->show();
                    }
                },
                this->managedConnections_);
        }

        layout.append(createCheckBox(
            "Play highlight sound even when Chatterino is focused",
            getSettings()->highlightAlwaysPlaySound));
        layout.append(createCheckBox(
            "Flash taskbar only stops highlighting when Chatterino is focused",
            getSettings()->longAlerts));
    }

    // ---- misc
    this->disabledUsersChangedTimer_.setSingleShot(true);
}

void HighlightingPage::openSoundDialog(const QModelIndex &clicked,
                                       EditableModelView *view, int soundColumn)
{
    auto fileUrl = QFileDialog::getOpenFileUrl(this, tr("Open Sound"), QUrl(),
                                               tr("Audio Files (*.mp3 *.wav)"));
    view->getModel()->setData(clicked, fileUrl, Qt::UserRole);
    view->getModel()->setData(clicked, fileUrl.fileName(), Qt::DisplayRole);
}

void HighlightingPage::openColorDialog(const QModelIndex &clicked,
                                       EditableModelView *view,
                                       HighlightTab tab)
{
    auto initial =
        view->getModel()->data(clicked, Qt::DecorationRole).value<QColor>();

    auto *dialog = new ColorPickerDialog(initial, this);
    // TODO: The QModelIndex clicked is technically not safe to persist here since the model
    // can be changed between the color dialog being created & the color dialog being closed
    QObject::connect(dialog, &ColorPickerDialog::colorConfirmed, this,
                     [=](auto selected) {
                         if (selected.isValid())
                         {
                             view->getModel()->setData(clicked, selected,
                                                       Qt::DecorationRole);
                         }
                     });
    dialog->show();
}

void HighlightingPage::tableCellClicked(const QModelIndex &clicked,
                                        EditableModelView *view,
                                        HighlightTab tab)
{
    if (!clicked.flags().testFlag(Qt::ItemIsEnabled))
    {
        return;
    }

    switch (tab)
    {
        case HighlightTab::Messages:
        case HighlightTab::Users: {
            using Column = HighlightModel::Column;

            if (clicked.column() == Column::SoundPath)
            {
                this->openSoundDialog(clicked, view, Column::SoundPath);
            }
            else if (clicked.column() == Column::Color)
            {
                this->openColorDialog(clicked, view, tab);
            }
        }
        break;

        case HighlightTab::Badges: {
            using Column = BadgeHighlightModel::Column;
            if (clicked.column() == Column::SoundPath)
            {
                this->openSoundDialog(clicked, view, Column::SoundPath);
            }
            else if (clicked.column() == Column::Color)
            {
                this->openColorDialog(clicked, view, tab);
            }
        }
        break;
    }
}

bool HighlightingPage::filterElements(const QString &query)
{
    std::array fields{0};

    bool matchMessages =
        this->viewMessages_->filterSearchResults(query, fields);
    this->tabWidget_->setTabVisible(0, matchMessages);

    bool matchUsers = this->viewUsers_->filterSearchResults(query, fields);
    this->tabWidget_->setTabVisible(1, matchUsers);

    bool matchBadges = this->viewBadges_->filterSearchResults(query, fields);
    this->tabWidget_->setTabVisible(2, matchBadges);

    bool matchBlacklistedUsers =
        this->viewBlacklistedUsers_->filterSearchResults(query, fields);
    this->tabWidget_->setTabVisible(3, matchBlacklistedUsers);

    return matchMessages || matchUsers || matchBadges || matchBlacklistedUsers;
}

}  // namespace chatterino
