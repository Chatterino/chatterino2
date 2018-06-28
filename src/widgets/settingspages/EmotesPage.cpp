#include "EmotesPage.hpp"

#include "util/LayoutCreator.hpp"

namespace chatterino {

EmotesPage::EmotesPage()
    : SettingsPage("Emotes", ":/images/emote.svg")
{
    //    SettingManager &settings = SettingManager::getInstance();
    //    LayoutCreator<EmotesPage> layoutCreator(this);
    //    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();

    //    // clang-format off
    //    layout.append(this->createCheckBox("Enable Twitch emotes", settings.enableTwitchEmotes));
    //    layout.append(this->createCheckBox("Enable BetterTTV emotes", settings.enableBttvEmotes));
    //    layout.append(this->createCheckBox("Enable FrankerFaceZ emotes",
    //    settings.enableFfzEmotes)); layout.append(this->createCheckBox("Enable emojis",
    //    settings.enableEmojis)); layout.append(this->createCheckBox("Enable gif animations",
    //    settings.enableGifAnimations));
    //    // clang-format on

    //    layout->addStretch(1);
}

}  // namespace chatterino
