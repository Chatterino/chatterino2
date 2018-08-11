#include "BrowserExtensionPage.hpp"

#include "util/LayoutCreator.hpp"

#include <QLabel>

#define CHROME_EXTENSION_LINK                                           \
    "https://chrome.google.com/webstore/detail/chatterino-native-host/" \
    "glknmaideaikkmemifbfkhnomoknepka"
#define FIREFOX_EXTENSION_LINK \
    "https://addons.mozilla.org/en-US/firefox/addon/chatterino-native-host/"

namespace chatterino {

BrowserExtensionPage::BrowserExtensionPage()
    : SettingsPage("Browser integration", "")
{
    auto layout =
        LayoutCreator<BrowserExtensionPage>(this).setLayoutType<QVBoxLayout>();

    auto label = layout.emplace<QLabel>(
        "The browser extension will replace the default Twitch.tv chat with "
        "chatterino while chatterino is running.");
    label->setWordWrap(true);

    auto chrome = layout.emplace<QLabel>("<a href=\"" CHROME_EXTENSION_LINK
                                         "\">Download for Google Chrome</a>");
    chrome->setOpenExternalLinks(true);
    auto firefox =
        layout.emplace<QLabel>("<a href=\"" FIREFOX_EXTENSION_LINK
                               "\">Download for Mozilla Firefox</a>");
    firefox->setOpenExternalLinks(true);
    layout->addStretch(1);
}

}  // namespace chatterino
