#include "ignoreuserspage.hpp"

#include "application.hpp"
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

    auto group = layout.emplace<QGroupBox>("Ignored users").setLayoutType<QVBoxLayout>();
    {
        group.append(this->createCheckBox("Enable twitch ignored users",
                                          app->settings->enableTwitchIgnoredUsers));

        auto anyways = group.emplace<QHBoxLayout>().withoutMargin();
        {
            anyways.emplace<QLabel>("Show anyways if:");
            anyways.emplace<QComboBox>();
            anyways->addStretch(1);
        }

        auto addremove = group.emplace<QHBoxLayout>().withoutMargin();
        {
            auto add = addremove.emplace<QPushButton>("Ignore user");
            auto remove = addremove.emplace<QPushButton>("Unignore User");
            UNUSED(add);     // TODO: Add on-clicked event
            UNUSED(remove);  // TODO: Add on-clicked event
            addremove->addStretch(1);
        }

        auto userList = group.emplace<QListView>();
        UNUSED(userList);  // TODO: Fill this list in with ignored users
    }

    auto label = layout.emplace<QLabel>(INFO);
    label->setWordWrap(true);
    label->setStyleSheet("color: #BBB");
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
