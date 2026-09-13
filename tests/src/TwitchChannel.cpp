// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/twitch/TwitchChannel.hpp"

#include "controllers/accounts/AccountController.hpp"
#include "messages/Message.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/Logging.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "Test.hpp"

#include <QDateTime>
#include <QString>

#include <memory>
#include <vector>

namespace chatterino {

namespace {

class LiveUpdateApplication : public mock::BaseApplication
{
public:
    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

    ITwitchIrcServer *getTwitch() override
    {
        return &this->twitch;
    }

    ILogging *getChatLogger() override
    {
        return &this->logging;
    }

    AccountController accounts;
    mock::MockTwitchIrcServer twitch;
    mock::EmptyLogging logging;
};

}  // namespace

TEST(TwitchChannel, LiveUpdateGrouping)
{
    LiveUpdateApplication app;
    auto channel = std::make_shared<TwitchChannel>("testchannel");
    const auto addEmote = [&](const QString &name, const QDateTime &time) {
        channel->addOrReplaceLiveUpdatesAddRemove(true, "BTTV", {}, name, time);
    };
    const auto time =
        QDateTime::fromString("1984-04-20T21:37:00Z", Qt::ISODate);

    addEmote("FirstEmote", time);
    ASSERT_EQ(channel->countMessages(), 1);
    auto first = channel->getLastMessage();
    EXPECT_EQ(first->serverReceivedTime, time);

    // Grouping considers the date, not only the time.
    const auto nextDay = time.addDays(1);
    addEmote("SecondEmote", nextDay);
    ASSERT_EQ(channel->countMessages(), 2);
    EXPECT_EQ(channel->getMessageSnapshot().at(0), first);
    EXPECT_FALSE(first->messageText.contains("SecondEmote"));

    // An update less than 5 seconds later joins the group.
    addEmote("ThirdEmote", nextDay.addSecs(4));
    ASSERT_EQ(channel->countMessages(), 2);
    auto grouped = channel->getLastMessage();
    EXPECT_TRUE(grouped->messageText.contains("SecondEmote"));
    EXPECT_TRUE(grouped->messageText.contains("ThirdEmote"));
    EXPECT_EQ(grouped->serverReceivedTime, nextDay.addSecs(4));

    // Updates more than five seconds apart start a new group.
    addEmote("FourthEmote", nextDay.addSecs(10));
    EXPECT_EQ(channel->countMessages(), 3);
}

}  // namespace chatterino

namespace chatterino::detail {

TEST(TwitchChannelDetail_isUnknownCommand, good)
{
    // clang-format off
    std::vector<QString> cases{
        "/me hello",
        ".me hello",
        "/ hello",
        ". hello",
        "/ /hello",
        ". .hello",
        "/ .hello",
        ". /hello",
        ".", // this results in an empty message but not in an error (twitchdev/issues#1019)
        "/me",
        ".me",
        "..",
        "...",
        "....",
        "",
        "foo",
        "a",
        "!",
        ". .",
        ". ..",
        ".. ..",
        ".. .",
        "/ /",
        "/ .",
        ". /",
        ". ./",
        ".. /",
        ".. me",
        ". me",
    };
    // clang-format on

    for (const auto &input : cases)
    {
        ASSERT_FALSE(isUnknownCommand(input))
            << input << " should not be considered an unknown command";
    }
}

TEST(TwitchChannelDetail_isUnknownCommand, bad)
{
    // clang-format off
    std::vector<QString> cases{
        "/badcommand",
        ".badcommand",
        "/badcommand hello",
        ".badcommand hello",
        "/@badcommand hello",
        ".@badcommand hello",
        "/bann username ban reason",
        "/bann username",
        "//",
        "./",
        "./me",
        "./w",
        "/.",
        "/.me",
        "/.w",
        "/,me",
    };
    // clang-format on

    for (const auto &input : cases)
    {
        ASSERT_TRUE(isUnknownCommand(input))
            << input << " should be considered an unknown command";
    }
}

}  // namespace chatterino::detail
