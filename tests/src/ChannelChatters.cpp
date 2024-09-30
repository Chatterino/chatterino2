#include "common/ChannelChatters.hpp"

#include "mocks/BaseApplication.hpp"
#include "mocks/Channel.hpp"
#include "mocks/Logging.hpp"
#include "Test.hpp"

#include <QColor>
#include <QStringList>

using namespace chatterino;
using chatterino::mock::MockChannel;

namespace {

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication() = default;

    ILogging *getChatLogger() override
    {
        return &this->logging;
    }

    mock::EmptyLogging logging;
};

}  // namespace

// Ensure inserting the same user does not increase the size of the stored colors
TEST(ChannelChatters, insertSameUser)
{
    MockApplication app;

    MockChannel channel("test");

    ChannelChatters chatters(channel);

    EXPECT_EQ(chatters.colorsSize(), 0);
    chatters.setUserColor("pajlada", QColor("#fff"));
    EXPECT_EQ(chatters.colorsSize(), 1);
    chatters.setUserColor("pajlada", QColor("#fff"));
    EXPECT_EQ(chatters.colorsSize(), 1);
}

// Ensure we can update a chatters color
TEST(ChannelChatters, insertSameUserUpdatesColor)
{
    MockApplication app;

    MockChannel channel("test");

    ChannelChatters chatters(channel);

    chatters.setUserColor("pajlada", QColor("#fff"));
    EXPECT_EQ(chatters.getUserColor("pajlada"), QColor("#fff"));
    chatters.setUserColor("pajlada", QColor("#f0f"));
    EXPECT_EQ(chatters.getUserColor("pajlada"), QColor("#f0f"));
}

// Ensure getting a non-existant users color returns an invalid QColor
TEST(ChannelChatters, getNonExistantUser)
{
    MockApplication app;

    MockChannel channel("test");

    ChannelChatters chatters(channel);

    EXPECT_EQ(chatters.getUserColor("nonexistantuser"), QColor());
}

// Ensure getting a user doesn't create an entry
TEST(ChannelChatters, getDoesNotCreate)
{
    MockApplication app;

    MockChannel channel("test");

    ChannelChatters chatters(channel);

    EXPECT_EQ(chatters.colorsSize(), 0);
    chatters.getUserColor("nonexistantuser");
    EXPECT_EQ(chatters.colorsSize(), 0);
}

// Ensure the least recently used entry is purged when we reach MAX_SIZE
TEST(ChannelChatters, insertMaxSize)
{
    MockApplication app;

    MockChannel channel("test");

    ChannelChatters chatters(channel);

    // Prime chatters with 2 control entries
    chatters.setUserColor("pajlada", QColor("#f00"));
    chatters.setUserColor("zneix", QColor("#f0f"));

    EXPECT_EQ(chatters.getUserColor("pajlada"), QColor("#f00"));
    EXPECT_EQ(chatters.getUserColor("zneix"), QColor("#f0f"));
    EXPECT_EQ(chatters.getUserColor("nonexistantuser"), QColor());

    EXPECT_EQ(chatters.colorsSize(), 2);

    for (int i = 0; i < ChannelChatters::maxChatterColorCount - 1; ++i)
    {
        auto username = QString("user%1").arg(i);
        chatters.setUserColor(username, QColor("#00f"));
    }

    // Should have bumped ONE entry out (pajlada)

    EXPECT_EQ(chatters.getUserColor("pajlada"), QColor());
    EXPECT_EQ(chatters.getUserColor("zneix"), QColor("#f0f"));
    EXPECT_EQ(chatters.getUserColor("user1"), QColor("#00f"));

    chatters.setUserColor("newuser", QColor("#00e"));

    for (int i = 0; i < ChannelChatters::maxChatterColorCount; ++i)
    {
        auto username = QString("user%1").arg(i);
        chatters.setUserColor(username, QColor("#00f"));
    }

    // One more entry should be bumped out (zneix)

    EXPECT_EQ(chatters.getUserColor("pajlada"), QColor());
    EXPECT_EQ(chatters.getUserColor("zneix"), QColor());
    EXPECT_EQ(chatters.getUserColor("user1"), QColor("#00f"));
}
