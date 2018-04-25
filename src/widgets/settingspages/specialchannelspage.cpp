#include "specialchannelspage.hpp"

#include "singletons/settingsmanager.hpp"
#include "util/layoutcreator.hpp"

#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

namespace chatterino {
namespace widgets {
namespace settingspages {

SpecialChannelsPage::SpecialChannelsPage()
    : SettingsPage("Special channels", "")
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    util::LayoutCreator<SpecialChannelsPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    auto mentions = layout.emplace<QGroupBox>("Mentions channel").setLayoutType<QVBoxLayout>();
    {
        mentions.emplace<QLabel>("Join /mentions to view your mentions.");
    }

    auto whispers = layout.emplace<QGroupBox>("Whispers").setLayoutType<QVBoxLayout>();
    {
        whispers.emplace<QLabel>("Join /whispers to view your mentions.");
    }

    layout->addStretch(1);
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
