#include "util/IncognitoBrowser.hpp"

#include "Test.hpp"

using namespace chatterino;

TEST(IncognitoBrowser, getPrivateSwitch)
{
    using namespace chatterino::incognitobrowser::detail;

    ASSERT_EQ(getPrivateSwitch("firefox.exe"), "-private-window");
    ASSERT_EQ(getPrivateSwitch("firefox"), "-private-window");
    ASSERT_EQ(getPrivateSwitch("firefox-forsen-version"), "-private-window");

    ASSERT_EQ(getPrivateSwitch("chrome.exe"), "-incognito");
    ASSERT_EQ(getPrivateSwitch("google-chrome-stable"), "-incognito");

    ASSERT_EQ(getPrivateSwitch("opera.exe"), "-newprivatetab");

    ASSERT_EQ(getPrivateSwitch("unsupportedBrowser.exe"), "");
}
