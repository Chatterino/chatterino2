// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "common/ChannelChatters.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/Channel.hpp"
#include "mocks/Logging.hpp"
#include "mocks/UserData.hpp"
#include "providers/twitch/UserColor.hpp"
#include "Test.hpp"

#include <QString>

namespace chatterino::twitch {

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

class TwitchUserColor : public ::testing::Test
{
protected:
    void SetUp() override
    {
        this->app = std::make_unique<MockApplication>();
        this->userDataController = std::make_unique<mock::UserDataController>();
        this->channel = std::make_unique<mock::MockChannel>("test");
        this->chatters = std::make_unique<ChannelChatters>(*this->channel);
    }

    void TearDown() override
    {
        this->chatters.reset();
        this->channel.reset();
        this->userDataController.reset();
        this->app.reset();
    }

    std::unique_ptr<MockApplication> app;
    std::unique_ptr<mock::UserDataController> userDataController;
    std::unique_ptr<mock::MockChannel> channel;
    std::unique_ptr<ChannelChatters> chatters;
};

TEST_F(TwitchUserColor, NoDataForUser)
{
    ASSERT_EQ(getUserColor({
                  .userLogin = "pajlada",
                  .userID = "11148817",
                  .userDataController = userDataController.get(),
                  .channelChatters = chatters.get(),
                  .color = {},
              }),
              std::nullopt);
}

TEST_F(TwitchUserColor, ChannelChattersData)
{
    // User exists in channel chatters.
    // Their color should be fetched from there.

    chatters->setUserColor("pajlada", QColor("#FF0000"));

    ASSERT_EQ(getUserColor({
                  .userLogin = "pajlada",
                  .userID = "11148817",
                  .userDataController = userDataController.get(),
                  .channelChatters = chatters.get(),
                  .color = {},
              }),
              QColor("#FF0000"));
}

TEST_F(TwitchUserColor, ChannelChattersDataButBaseColor)
{
    // User exists in channel chatters
    // The base color is valid and should be used.

    chatters->setUserColor("pajlada", QColor("#FF0000"));

    ASSERT_EQ(getUserColor({
                  .userLogin = "pajlada",
                  .userID = "11148817",
                  .userDataController = userDataController.get(),
                  .channelChatters = chatters.get(),
                  .color = QColor("#00FF00"),
              }),
              QColor("#00FF00"));
}

TEST_F(TwitchUserColor, ChannelChattersDataButBaseColorInvalid)
{
    // User exists in channel chatters
    // A base color has been defined but it's invalid
    // The channel chatter color should be used

    ASSERT_EQ(0, chatters->colorsSize());
    chatters->setUserColor("pajlada", QColor("#FF0000"));

    ASSERT_EQ(getUserColor({
                  .userLogin = "pajlada",
                  .userID = "11148817",
                  .userDataController = userDataController.get(),
                  .channelChatters = chatters.get(),
                  .color = QColor("this is an invalid color"),
              }),
              QColor("#FF0000"));
}

TEST_F(TwitchUserColor, UCD)
{
    // User exists in channel chatters
    // A base color has been defined
    // User exists in user data controller
    // The user data controller color should be used

    chatters->setUserColor("pajlada", QColor("#FF0000"));

    userDataController->setUserColor("11148817", "#0000FF");

    ASSERT_EQ(getUserColor({
                  .userLogin = "pajlada",
                  .userID = "11148817",
                  .userDataController = userDataController.get(),
                  .channelChatters = chatters.get(),
                  .color = QColor("#00FF00"),
              }),
              QColor("#0000FF"));
}

}  // namespace chatterino::twitch
