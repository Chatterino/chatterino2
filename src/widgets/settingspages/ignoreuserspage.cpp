#include "ignoreuserspage.hpp"

#include "singletons/settingsmanager.hpp"
#include "util/layoutcreator.hpp"

#include <QCheckBox>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QVBoxLayout>

// clang-format off
#define INFO "/ignore <user> in chat ignores a user\n/unignore <user> in chat unignores a user\n\nChatterino uses the twitch api for ignored users so they are shared with the webchat.\nIf you use your own oauth key make sure that it has the correct permissions."
// clang-format on

namespace chatterino {
namespace widgets {
namespace settingspages {
IgnoreUsersPage::IgnoreUsersPage()
    : SettingsPage("Ignore Users", ":/images/theme.svg")
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    util::LayoutCreator<IgnoreUsersPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>().withoutMargin();

    auto label = layout.emplace<QLabel>(INFO);
    label->setWordWrap(true);
    label->setStyleSheet("color: #BBB");

    layout.append(
        this->createCheckBox("Enable twitch ignored users", settings.enableTwitchIgnoredUsers));

    auto anyways = layout.emplace<QHBoxLayout>().withoutMargin();
    {
        anyways.emplace<QLabel>("Show anyways if:");
        anyways.emplace<QComboBox>();
        anyways->addStretch(1);
    }

    auto addremove = layout.emplace<QHBoxLayout>().withoutMargin();
    {
        auto add = addremove.emplace<QPushButton>("Ignore user");
        auto remove = addremove.emplace<QPushButton>("Unignore User");
        addremove->addStretch(1);
    }

    auto userList = layout.emplace<QListView>();
}
}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
