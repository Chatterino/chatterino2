#include "SpecialChannelsPage.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "util/LayoutCreator.hpp"

#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

namespace chatterino {

SpecialChannelsPage::SpecialChannelsPage()
    : SettingsPage("Special channels", "")
{
    LayoutCreator<SpecialChannelsPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    auto mentions = layout.emplace<QGroupBox>("Mentions channel")
                        .setLayoutType<QVBoxLayout>();
    {
        mentions.emplace<QLabel>("Join /mentions to view your mentions.");
    }

    auto whispers =
        layout.emplace<QGroupBox>("Whispers").setLayoutType<QVBoxLayout>();
    {
        whispers.emplace<QLabel>("Join /whispers to view your mentions.");
    }

    layout->addStretch(1);
}

}  // namespace chatterino
