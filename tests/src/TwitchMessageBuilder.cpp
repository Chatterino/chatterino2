#include "providers/twitch/TwitchMessageBuilder.hpp"

#include "common/Channel.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchBadge.hpp"

#include "ircconnection.h"

#include <gtest/gtest.h>
#include <QDebug>
#include <QString>
#include <unordered_map>
#include <vector>

using namespace chatterino;

TEST(TwitchMessageBuilder, CommaSeparatedListTagParsing)
{
    struct TestCase {
        QString input;
        std::pair<QString, QString> expectedOutput;
    };

    std::vector<TestCase> testCases{
        {
            "broadcaster/1",
            {"broadcaster", "1"},
        },
        {
            "predictions/foo/bar/baz",
            {"predictions", "foo/bar/baz"},
        },
        {
            "test/",
            {"test", ""},
        },
        {
            "/",
            {"", ""},
        },
        {
            "/value",
            {"", "value"},
        },
        {
            "",
            {"", ""},
        },
    };

    for (const auto &test : testCases)
    {
        auto output = TwitchMessageBuilder::slashKeyValue(test.input);

        EXPECT_EQ(output, test.expectedOutput)
            << "Input " << test.input.toStdString() << " failed";
    }
}

TEST(TwitchMessageBuilder, BadgeInfoParsing)
{
    struct TestCase {
        QByteArray input;
        std::unordered_map<QString, QString> expectedBadgeInfo;
        std::vector<Badge> expectedBadges;
    };

    std::vector<TestCase> testCases{
        {
            R"(@badge-info=predictions/<<<<<<\sHEAD[15A⸝asdf/test;badges=predictions/pink-2;client-nonce=9dbb88e516edf4efb055c011f91ea0cf;color=#FF4500;display-name=もっと頑張って;emotes=;first-msg=0;flags=;id=feb00b12-4ec5-4f77-9160-667de463dab1;mod=0;room-id=99631238;subscriber=0;tmi-sent-ts=1653494874297;turbo=0;user-id=648946956;user-type= :zniksbot!zniksbot@zniksbot.tmi.twitch.tv PRIVMSG #zneix :-tags")",
            {
                {"predictions", R"(<<<<<<\sHEAD[15A⸝asdf/test)"},
            },
            {
                Badge{"predictions", "pink-2"},
            },
        },
        {
            R"(@badge-info=predictions/<<<<<<\sHEAD[15A⸝asdf/test,founder/17;badges=predictions/pink-2,vip/1,founder/0,bits/1;client-nonce=9b836e232170a9df213aefdcb458b67e;color=#696969;display-name=NotKarar;emotes=;first-msg=0;flags=;id=e00881bd-5f21-4993-8bbd-1736cd13d42e;mod=0;room-id=99631238;subscriber=1;tmi-sent-ts=1653494879409;turbo=0;user-id=89954186;user-type= :notkarar!notkarar@notkarar.tmi.twitch.tv PRIVMSG #zneix :-tags)",
            {
                {"predictions", R"(<<<<<<\sHEAD[15A⸝asdf/test)"},
                {"founder", "17"},
            },
            {
                Badge{"predictions", "pink-2"},
                Badge{"vip", "1"},
                Badge{"founder", "0"},
                Badge{"bits", "1"},
            },
        },
        {
            R"(@badge-info=predictions/foo/bar/baz;badges=predictions/blue-1,moderator/1,glhf-pledge/1;client-nonce=f73f16228e6e32f8e92b47ab8283b7e1;color=#1E90FF;display-name=zneixbot;emotes=30259:6-12;first-msg=0;flags=;id=9682a5f1-a0b0-45e2-be9f-8074b58c5f8f;mod=1;room-id=99631238;subscriber=0;tmi-sent-ts=1653573594035;turbo=0;user-id=463521670;user-type=mod :zneixbot!zneixbot@zneixbot.tmi.twitch.tv PRIVMSG #zneix :-tags HeyGuys)",
            {
                {"predictions", "foo/bar/baz"},
            },
            {
                Badge{"predictions", "blue-1"},
                Badge{"moderator", "1"},
                Badge{"glhf-pledge", "1"},
            },
        },
        {
            R"(@badge-info=subscriber/22;badges=broadcaster/1,subscriber/18,glhf-pledge/1;color=#F97304;display-name=zneix;emotes=;first-msg=0;flags=;id=1d99f67f-a566-4416-a4e2-e85d7fce9223;mod=0;room-id=99631238;subscriber=1;tmi-sent-ts=1653612232758;turbo=0;user-id=99631238;user-type= :zneix!zneix@zneix.tmi.twitch.tv PRIVMSG #zneix :-tags)",
            {
                {"subscriber", "22"},
            },
            {
                Badge{"broadcaster", "1"},
                Badge{"subscriber", "18"},
                Badge{"glhf-pledge", "1"},
            },
        },
    };

    for (const auto &test : testCases)
    {
        auto privmsg =
            Communi::IrcPrivateMessage::fromData(test.input, nullptr);

        auto outputBadgeInfo =
            TwitchMessageBuilder::parseBadgeInfoTag(privmsg->tags());
        EXPECT_EQ(outputBadgeInfo, test.expectedBadgeInfo)
            << "Input for badgeInfo " << test.input.toStdString() << " failed";

        auto outputBadges =
            SharedMessageBuilder::parseBadgeTag(privmsg->tags());
        EXPECT_EQ(outputBadges, test.expectedBadges)
            << "Input for badges " << test.input.toStdString() << " failed";
    }
}
