#include "ignoreuserspage.hpp"

#include "singletons/settingsmanager.hpp"
#include "util/layoutcreator.hpp"

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QVBoxLayout>

// clang-format off
#define INFO "/ignore <user> in chat ignores a user\n/unignore <user> in chat unignores a user\n\nChatterino uses the twitch api for ignored users so they are shared with the webchat.\nIf you use your own oauth key make sure that it has the correct permissions.\n"
// clang-format on

namespace chatterino {
namespace widgets {
namespace settingspages {

IgnoreUsersPage::IgnoreUsersPage()
    : SettingsPage("Ignore Users", "")
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    util::LayoutCreator<IgnoreUsersPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    auto label = layout.emplace<QLabel>(INFO);
    label->setWordWrap(true);
    label->setStyleSheet("color: #BBB");

    auto group = layout.emplace<QGroupBox>("Ignored users").setLayoutType<QVBoxLayout>();
    {
        group.append(
            this->createCheckBox("Enable twitch ignored users", settings.enableTwitchIgnoredUsers));

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
            addremove->addStretch(1);
        }

        auto userList = group.emplace<QListView>();
    }
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
