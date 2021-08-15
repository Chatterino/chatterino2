#include "common/ChannelChatters.hpp"

#include <gtest/gtest.h>
#include <QColor>
#include <QStringList>

namespace chatterino {

class MockChannel : public Channel
{
public:
    MockChannel(const QString &name)
        : Channel(name, Channel::Type::Twitch)
    {
    }
};

}  // namespace chatterino

using namespace chatterino;

// Ensure inserting the same user does not increase the size of the stored colors
TEST(ChatterChatters, insertSameUser)
{
    MockChannel channel("test");

    ChannelChatters chatters(channel);

    EXPECT_EQ(chatters.colorsSize(), 0);
    chatters.setUserColor("pajlada", QColor("#fff"));
    EXPECT_EQ(chatters.colorsSize(), 1);
    chatters.setUserColor("pajlada", QColor("#fff"));
    EXPECT_EQ(chatters.colorsSize(), 1);
}

// Ensure we can update a chatters color
TEST(ChatterChatters, insertSameUserUpdatesColor)
{
    MockChannel channel("test");

    ChannelChatters chatters(channel);

    chatters.setUserColor("pajlada", QColor("#fff"));
    EXPECT_EQ(chatters.getUserColor("pajlada"), QColor("#fff"));
    chatters.setUserColor("pajlada", QColor("#f0f"));
    EXPECT_EQ(chatters.getUserColor("pajlada"), QColor("#f0f"));
}

// Ensure getting a non-existant users color returns an invalid QColor
TEST(ChatterChatters, getNonExistantUser)
{
    MockChannel channel("test");

    ChannelChatters chatters(channel);

    EXPECT_EQ(chatters.getUserColor("nonexistantuser"), QColor());
}

// Ensure getting a user doesn't create an entry
TEST(ChatterChatters, getDoesNotCreate)
{
    MockChannel channel("test");

    ChannelChatters chatters(channel);

    EXPECT_EQ(chatters.colorsSize(), 0);
    chatters.getUserColor("nonexistantuser");
    EXPECT_EQ(chatters.colorsSize(), 0);
}

// Ensure the least recently used entry is purged when we reach MAX_SIZE
TEST(ChatterChatters, insertMaxSize)
{
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
