#include "controllers/moderationactions/ModerationAction.hpp"

#include "messages/Image.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/Emotes.hpp"
#include "singletons/Resources.hpp"
#include "Test.hpp"

#include <QString>

using namespace chatterino;

using namespace std::chrono_literals;

namespace {

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication() = default;

    IEmotes *getEmotes() override
    {
        return &this->emotes;
    }

    mock::Emotes emotes;
};

class ModerationActionTest : public ::testing::Test
{
public:
    MockApplication mockApplication;
};

}  // namespace

TEST_F(ModerationActionTest, Parse)
{
    struct TestCase {
        QString action;
        QString iconPath;

        QString expectedLine1;
        QString expectedLine2;

        std::optional<ImagePtr> expectedImage;

        ModerationAction::Type expectedType;
    };

    std::vector<TestCase> tests{
        {
            .action = "/ban forsen",
            .expectedImage =
                Image::fromResourcePixmap(getResources().buttons.ban),
            .expectedType = ModerationAction::Type::Ban,
        },
        {
            .action = "/delete {message.id}",
            .expectedImage =
                Image::fromResourcePixmap(getResources().buttons.trashCan),
            .expectedType = ModerationAction::Type::Delete,
        },
        {
            .action = "/timeout {user.name} 1d",
            .expectedLine1 = "1",
            .expectedLine2 = "d",
            .expectedType = ModerationAction::Type::Timeout,
        },
        {
            .action = ".timeout {user.name} 300",
            .expectedLine1 = "5",
            .expectedLine2 = "m",
            .expectedType = ModerationAction::Type::Timeout,
        },
        {
            .action = "forsen",
            .expectedLine1 = "fo",
            .expectedLine2 = "rs",
            .expectedType = ModerationAction::Type::Custom,
        },
        {
            .action = "forsen",
            .iconPath = "file:///this-is-the-path-to-the-icon.png",
            .expectedLine1 = "fo",
            .expectedLine2 = "rs",
            .expectedImage =
                Image::fromUrl(Url{"file:///this-is-the-path-to-the-icon.png"}),
            .expectedType = ModerationAction::Type::Custom,
        },
    };

    for (const auto &test : tests)
    {
        ModerationAction moderationAction(test.action, test.iconPath);

        EXPECT_EQ(moderationAction.getAction(), test.action);

        EXPECT_EQ(moderationAction.getLine1(), test.expectedLine1);
        EXPECT_EQ(moderationAction.getLine2(), test.expectedLine2);

        EXPECT_EQ(moderationAction.getImage(), test.expectedImage);

        EXPECT_EQ(moderationAction.getType(), test.expectedType);
    }
}
