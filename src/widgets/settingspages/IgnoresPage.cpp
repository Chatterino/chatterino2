#include "widgets/settingspages/IgnoresPage.hpp"

#include "Application.hpp"
#include "common/Literals.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/ignores/IgnoreModel.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchUser.hpp"
#include "singletons/Settings.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QCheckBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

namespace chatterino {

using namespace literals;

static void addPhrasesTab(LayoutCreator<QVBoxLayout> box);
static void addUsersTab(IgnoresPage &page, LayoutCreator<QVBoxLayout> box,
                        QStringListModel &model);

IgnoresPage::IgnoresPage()
{
    LayoutCreator<IgnoresPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    this->tabWidget_ = new QTabWidget();
    auto tabs = layout.append(this->tabWidget_);

    addPhrasesTab(tabs.appendTab(new QVBoxLayout, "Messages"));
    addUsersTab(*this, tabs.appendTab(new QVBoxLayout, "Users"),
                this->userListModel_);
    this->onShow();
}

void IgnoresPage::addPhrasesTab(LayoutCreator<QVBoxLayout> layout)
{
    layout.emplace<QLabel>("Ignore messages based certain patterns.");
    viewMessages_ = layout
                        .emplace<EditableModelView>(
                            (new IgnoreModel(nullptr))
                                ->initialized(&getSettings()->ignoredMessages))
                        .getElement();
    viewMessages_->setTitles(
        {"Pattern", "Regex", "Case-sensitive", "Block", "Replacement"});
    viewMessages_->getTableView()->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Fixed);
    viewMessages_->getTableView()->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    viewMessages_->addRegexHelpLink();

    QTimer::singleShot(1, [this] {
        viewMessages_->getTableView()->resizeColumnsToContents();
        viewMessages_->getTableView()->setColumnWidth(0, 200);
    });

    // We can safely ignore this signal connection since we own the viewMessages_
    std::ignore = viewMessages_->addButtonPressed.connect([] {
        getSettings()->ignoredMessages.append(
            IgnorePhrase{"my pattern", false, false,
                         getSettings()->ignoredPhraseReplace.getValue(), true});
    });
}

void addUsersTab(IgnoresPage &page, LayoutCreator<QVBoxLayout> users,
                 QStringListModel &userModel)
{
    auto label = users.emplace<QLabel>(
        u"/block <user> in chat blocks a user.\n/unblock <user> in chat unblocks a user.\nYou can also click on a user to open the usercard."_s);
    label->setWordWrap(true);
    users.append(page.createCheckBox("Enable Twitch blocked users",
                                     getSettings()->enableTwitchBlockedUsers));

    auto anyways = users.emplace<QHBoxLayout>().withoutMargin();
    {
        anyways.emplace<QLabel>("Show messages from blocked users:");

        auto *combo = anyways.emplace<QComboBox>().getElement();
        combo->addItems(
            {"Never", "If you are Moderator", "If you are Broadcaster"});

        auto &setting = getSettings()->showBlockedUsersMessages;

        setting.connect([combo](const int value) {
            combo->setCurrentIndex(value);
        });

        QObject::connect(combo,
                         QOverload<int>::of(&QComboBox::currentIndexChanged),
                         [&setting](int index) {
                             if (index != -1)
                             {
                                 setting = index;
                             }
                         });

        anyways->addStretch(1);
    }

    /*auto addremove = users.emplace<QHBoxLayout>().withoutMargin();
    {
        auto add = addremove.emplace<QPushButton>("Block user");
        auto remove = addremove.emplace<QPushButton>("Unblock User");
        addremove->addStretch(1);
    }*/

    users.emplace<QLabel>("List of blocked users:");
    users.emplace<QListView>()->setModel(&userModel);
}

void IgnoresPage::onShow()
{
    auto *app = getApp();

    auto user = app->getAccounts()->twitch.getCurrent();

    if (user->isAnon())
    {
        this->userListModel_.setStringList({});
        return;
    }

    QStringList users;
    users.reserve(user->blocks().size());

    for (const auto &blockedUser : user->blocks())
    {
        users << blockedUser.name;
    }
    users.sort(Qt::CaseInsensitive);
    this->userListModel_.setStringList(users);
}

bool IgnoresPage::filterElements(const QString &query)
{
    auto *fields = new std::vector<int>{0, 4};

    bool matchMessages = viewMessages_->filterSearchResults(query, *fields);
    this->tabWidget_->setTabVisible(0, matchMessages);

    return matchMessages;
}

}  // namespace chatterino
