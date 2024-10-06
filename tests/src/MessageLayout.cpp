#include "messages/layouts/MessageLayout.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/layouts/MessageLayoutContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "mocks/BaseApplication.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "Test.hpp"

#include <QDebug>
#include <QString>

#include <memory>

using namespace chatterino;

namespace {

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication()
        : windowManager(this->paths_, this->settings, this->theme, this->fonts)
    {
    }

    WindowManager *getWindows() override
    {
        return &this->windowManager;
    }

    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

    AccountController accounts;
    WindowManager windowManager;
};

constexpr int WIDTH = 300;

class MessageLayoutTest
{
public:
    // "aaaaaaaa bbbbbbbb cccccccc"
    MessageLayoutTest(const QString &text)
    {
        MessageBuilder builder;
        builder.append(
            std::make_unique<TextElement>(text, MessageElementFlag::Text));
        this->layout = std::make_unique<MessageLayout>(builder.release());
        MessageColors colors;
        this->layout->layout(
            {
                .messageColors = colors,
                .flags = MessageElementFlag::Text,
                .width = WIDTH,
                .scale = 1,
                .imageScale = 1,
            },
            false);
    }

    MockApplication mockApplication;
    std::unique_ptr<MessageLayout> layout;
};

}  // namespace

TEST(TextElement, BasicCase)
{
    auto test = MessageLayoutTest("abc");

    // Simulate we are clicking on the first word
    auto point = QPoint(WIDTH / 20, test.layout->getHeight() / 2);

    const auto *hoveredElement = test.layout->getElementAt(point);
    ASSERT_NE(hoveredElement, nullptr);

    const auto [wordStart, wordEnd] =
        test.layout->getWordBounds(hoveredElement, point);

    EXPECT_EQ(wordStart, 0);
    EXPECT_EQ(wordEnd, 3);
}
