#include "ignoreuserspage.hpp"

#include "application.hpp"
#include "singletons/accountmanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "util/layoutcreator.hpp"

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QVBoxLayout>

// clang-format off
#define INFO "/ignore <user> in chat ignores a user\n/unignore <user> in chat unignores a user"
// clang-format on

namespace chatterino {
namespace widgets {
namespace settingspages {

IgnoreUsersPage::IgnoreUsersPage()
    : SettingsPage("Ignores", "")
{
    auto app = getApp();

    util::LayoutCreator<IgnoreUsersPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    //    auto group = layout.emplace<QGroupBox>("Ignored users").setLayoutType<QVBoxLayout>();
    auto tabs = layout.emplace<QTabWidget>();
    tabs->setStyleSheet("color: #000");

    // users
    auto users = tabs.appendTab(new QVBoxLayout, "Users");
    {
        users.append(this->createCheckBox("Enable twitch ignored users",
                                          app->settings->enableTwitchIgnoredUsers));

        auto anyways = users.emplace<QHBoxLayout>().withoutMargin();
        {
            anyways.emplace<QLabel>("Show anyways if:");
            anyways.emplace<QComboBox>();
            anyways->addStretch(1);
        }

        auto addremove = users.emplace<QHBoxLayout>().withoutMargin();
        {
            auto add = addremove.emplace<QPushButton>("Ignore user");
            auto remove = addremove.emplace<QPushButton>("Unignore User");
            UNUSED(add);     // TODO: Add on-clicked event
            UNUSED(remove);  // TODO: Add on-clicked event
            addremove->addStretch(1);
        }

        users.emplace<QListView>()->setModel(&this->userListModel);
    }

    // messages
    auto messages = tabs.appendTab(new QVBoxLayout, "Messages");
    {
        messages.emplace<QLabel>("wip");
    }

    auto label = layout.emplace<QLabel>(INFO);
    label->setWordWrap(true);
    label->setStyleSheet("color: #BBB");
}

void IgnoreUsersPage::onShow()
{
    auto app = getApp();

    auto user = app->accounts->Twitch.getCurrent();

    if (user->isAnon()) {
        return;
    }

    QStringList users;
    for (const auto &ignoredUser : user->getIgnores()) {
        users << ignoredUser.name;
    }
    this->userListModel.setStringList(users);
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
