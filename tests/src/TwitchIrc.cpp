#include "providers/twitch/TwitchIrc.hpp"

#include "mocks/BaseApplication.hpp"
#include "mocks/Emotes.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "Test.hpp"
#include "util/IrcHelpers.hpp"

using namespace chatterino;

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

}  // namespace

class TestTwitchIrc : public ::testing::Test
{
protected:
    void SetUp() override
    {
        this->mockApplication = std::make_unique<MockApplication>();
    }

    void TearDown() override
    {
        this->mockApplication.reset();
    }

    std::unique_ptr<MockApplication> mockApplication;
};

TEST(TwitchIrc, CommaSeparatedListTagParsing)
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
        auto output = slashKeyValue(test.input);

        EXPECT_EQ(output, test.expectedOutput)
            << "Input " << test.input << " failed";
    }
}

TEST(TwitchIrc, BadgeInfoParsing)
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
        auto *privmsg =
            Communi::IrcPrivateMessage::fromData(test.input, nullptr);

        auto outputBadgeInfo = parseBadgeInfoTag(privmsg->tags());
        EXPECT_EQ(outputBadgeInfo, test.expectedBadgeInfo)
            << "Input for badgeInfo " << test.input << " failed";

        auto outputBadges = parseBadgeTag(privmsg->tags());
        EXPECT_EQ(outputBadges, test.expectedBadges)
            << "Input for badges " << test.input << " failed";

        delete privmsg;
    }
}

TEST_F(TestTwitchIrc, ParseTwitchEmotes)
{
    struct TestCase {
        QByteArray input;
        std::vector<TwitchEmoteOccurrence> expectedTwitchEmotes;
    };

    auto *twitchEmotes = this->mockApplication->getEmotes()->getTwitchEmotes();

    std::vector<TestCase> testCases{
        {
            // action /me message
            R"(@badge-info=subscriber/80;badges=broadcaster/1,subscriber/3072,partner/1;color=#CC44FF;display-name=pajlada;emote-only=1;emotes=25:0-4;first-msg=0;flags=;id=90ef1e46-8baa-4bf2-9c54-272f39d6fa11;mod=0;returning-chatter=0;room-id=11148817;subscriber=1;tmi-sent-ts=1662206235860;turbo=0;user-id=11148817;user-type= :pajlada!pajlada@pajlada.tmi.twitch.tv PRIVMSG #pajlada :ACTION Kappa)",
            {
                {{
                    0,  // start
                    4,  // end
                    twitchEmotes->getOrCreateEmote(EmoteId{"25"},
                                                   EmoteName{"Kappa"}),  // ptr
                    EmoteName{"Kappa"},                                  // name
                }},
            },
        },
        {
            R"(@badge-info=subscriber/17;badges=subscriber/12,no_audio/1;color=#EBA2C0;display-name=jammehcow;emote-only=1;emotes=25:0-4;first-msg=0;flags=;id=9c2dd916-5a6d-4c1f-9fe7-a081b62a9c6b;mod=0;returning-chatter=0;room-id=11148817;subscriber=1;tmi-sent-ts=1662201093248;turbo=0;user-id=82674227;user-type= :jammehcow!jammehcow@jammehcow.tmi.twitch.tv PRIVMSG #pajlada :Kappa)",
            {
                {{
                    0,  // start
                    4,  // end
                    twitchEmotes->getOrCreateEmote(EmoteId{"25"},
                                                   EmoteName{"Kappa"}),  // ptr
                    EmoteName{"Kappa"},                                  // name
                }},
            },
        },
        {
            R"(@badge-info=;badges=no_audio/1;color=#DAA520;display-name=Mm2PL;emote-only=1;emotes=1902:0-4;first-msg=0;flags=;id=9b1c3cb9-7817-47ea-add1-f9d4a9b4f846;mod=0;returning-chatter=0;room-id=11148817;subscriber=0;tmi-sent-ts=1662201095690;turbo=0;user-id=117691339;user-type= :mm2pl!mm2pl@mm2pl.tmi.twitch.tv PRIVMSG #pajlada :Keepo)",
            {
                {{
                    0,  // start
                    4,  // end
                    twitchEmotes->getOrCreateEmote(EmoteId{"1902"},
                                                   EmoteName{"Keepo"}),  // ptr
                    EmoteName{"Keepo"},                                  // name
                }},
            },
        },
        {
            R"(@badge-info=;badges=no_audio/1;color=#DAA520;display-name=Mm2PL;emote-only=1;emotes=25:0-4/1902:6-10/305954156:12-19;first-msg=0;flags=;id=7be87072-bf24-4fa3-b3df-0ea6fa5f1474;mod=0;returning-chatter=0;room-id=11148817;subscriber=0;tmi-sent-ts=1662201102276;turbo=0;user-id=117691339;user-type= :mm2pl!mm2pl@mm2pl.tmi.twitch.tv PRIVMSG #pajlada :Kappa Keepo PogChamp)",
            {
                {
                    {
                        0,  // start
                        4,  // end
                        twitchEmotes->getOrCreateEmote(
                            EmoteId{"25"}, EmoteName{"Kappa"}),  // ptr
                        EmoteName{"Kappa"},                      // name
                    },
                    {
                        6,   // start
                        10,  // end
                        twitchEmotes->getOrCreateEmote(
                            EmoteId{"1902"}, EmoteName{"Keepo"}),  // ptr
                        EmoteName{"Keepo"},                        // name
                    },
                    {
                        12,  // start
                        19,  // end
                        twitchEmotes->getOrCreateEmote(
                            EmoteId{"305954156"},
                            EmoteName{"PogChamp"}),  // ptr
                        EmoteName{"PogChamp"},       // name
                    },
                },
            },
        },
        {
            R"(@badge-info=subscriber/80;badges=broadcaster/1,subscriber/3072,partner/1;color=#CC44FF;display-name=pajlada;emote-only=1;emotes=25:0-4,6-10;first-msg=0;flags=;id=f7516287-e5d1-43ca-974e-fe0cff84400b;mod=0;returning-chatter=0;room-id=11148817;subscriber=1;tmi-sent-ts=1662204375009;turbo=0;user-id=11148817;user-type= :pajlada!pajlada@pajlada.tmi.twitch.tv PRIVMSG #pajlada :Kappa Kappa)",
            {
                {
                    {
                        0,  // start
                        4,  // end
                        twitchEmotes->getOrCreateEmote(
                            EmoteId{"25"}, EmoteName{"Kappa"}),  // ptr
                        EmoteName{"Kappa"},                      // name
                    },
                    {
                        6,   // start
                        10,  // end
                        twitchEmotes->getOrCreateEmote(
                            EmoteId{"25"}, EmoteName{"Kappa"}),  // ptr
                        EmoteName{"Kappa"},                      // name
                    },
                },
            },
        },
        {
            R"(@badge-info=subscriber/80;badges=broadcaster/1,subscriber/3072,partner/1;color=#CC44FF;display-name=pajlada;emotes=25:0-4,8-12;first-msg=0;flags=;id=44f85d39-b5fb-475d-8555-f4244f2f7e82;mod=0;returning-chatter=0;room-id=11148817;subscriber=1;tmi-sent-ts=1662204423418;turbo=0;user-id=11148817;user-type= :pajlada!pajlada@pajlada.tmi.twitch.tv PRIVMSG #pajlada :Kappa 😂 Kappa)",
            {
                {
                    {
                        0,  // start
                        4,  // end
                        twitchEmotes->getOrCreateEmote(
                            EmoteId{"25"}, EmoteName{"Kappa"}),  // ptr
                        EmoteName{"Kappa"},                      // name
                    },
                    {
                        9,   // start - modified due to emoji
                        13,  // end - modified due to emoji
                        twitchEmotes->getOrCreateEmote(
                            EmoteId{"25"}, EmoteName{"Kappa"}),  // ptr
                        EmoteName{"Kappa"},                      // name
                    },
                },
            },
        },
        {
            // start out of range
            R"(@emotes=84608:9-10 :test!test@test.tmi.twitch.tv PRIVMSG #pajlada :foo bar)",
            {},
        },
        {
            // one character emote
            R"(@emotes=84608:0-0 :test!test@test.tmi.twitch.tv PRIVMSG #pajlada :foo bar)",
            {
                {
                    0,  // start
                    0,  // end
                    twitchEmotes->getOrCreateEmote(EmoteId{"84608"},
                                                   EmoteName{"f"}),  // ptr
                    EmoteName{"f"},                                  // name
                },
            },
        },
        {
            // two character emote
            R"(@emotes=84609:0-1 :test!test@test.tmi.twitch.tv PRIVMSG #pajlada :foo bar)",
            {
                {
                    0,  // start
                    1,  // end
                    twitchEmotes->getOrCreateEmote(EmoteId{"84609"},
                                                   EmoteName{"fo"}),  // ptr
                    EmoteName{"fo"},                                  // name
                },
            },
        },
        {
            // end out of range
            R"(@emotes=84608:0-15 :test!test@test.tmi.twitch.tv PRIVMSG #pajlada :foo bar)",
            {},
        },
        {
            // range bad (end character before start)
            R"(@emotes=84608:15-2 :test!test@test.tmi.twitch.tv PRIVMSG #pajlada :foo bar)",
            {},
        },
    };

    for (const auto &test : testCases)
    {
        auto *privmsg = dynamic_cast<Communi::IrcPrivateMessage *>(
            Communi::IrcPrivateMessage::fromData(test.input, nullptr));
        ASSERT_NE(privmsg, nullptr);
        QString originalMessage = privmsg->content();

        // TODO: Add tests with replies
        auto actualTwitchEmotes =
            parseTwitchEmotes(privmsg->tags(), originalMessage, 0);

        EXPECT_EQ(actualTwitchEmotes, test.expectedTwitchEmotes)
            << "Input for twitch emotes " << test.input << " failed";

        delete privmsg;
    }
}
