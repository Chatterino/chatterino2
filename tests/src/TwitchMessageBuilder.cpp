#include "providers/twitch/TwitchMessageBuilder.hpp"

#include "common/Channel.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "messages/MessageBuilder.hpp"
#include "mocks/Channel.hpp"
#include "mocks/ChatterinoBadges.hpp"
#include "mocks/DisabledStreamerMode.hpp"
#include "mocks/EmptyApplication.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "mocks/UserData.hpp"
#include "providers/ffz/FfzBadges.hpp"
#include "providers/seventv/SeventvBadges.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "singletons/Emotes.hpp"
#include "Test.hpp"

#include <IrcConnection>
#include <QDebug>
#include <QString>

#include <unordered_map>
#include <vector>

using namespace chatterino;
using chatterino::mock::MockChannel;

namespace {

class MockApplication : mock::EmptyApplication
{
public:
    IEmotes *getEmotes() override
    {
        return &this->emotes;
    }

    IUserDataController *getUserData() override
    {
        return &this->userData;
    }

    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

    ITwitchIrcServer *getTwitch() override
    {
        return &this->twitch;
    }

    IChatterinoBadges *getChatterinoBadges() override
    {
        return &this->chatterinoBadges;
    }

    FfzBadges *getFfzBadges() override
    {
        return &this->ffzBadges;
    }

    SeventvBadges *getSeventvBadges() override
    {
        return &this->seventvBadges;
    }

    HighlightController *getHighlights() override
    {
        return &this->highlights;
    }

    BttvEmotes *getBttvEmotes() override
    {
        return &this->bttvEmotes;
    }

    FfzEmotes *getFfzEmotes() override
    {
        return &this->ffzEmotes;
    }

    SeventvEmotes *getSeventvEmotes() override
    {
        return &this->seventvEmotes;
    }

    IStreamerMode *getStreamerMode() override
    {
        return &this->streamerMode;
    }

    AccountController accounts;
    Emotes emotes;
    mock::UserDataController userData;
    mock::MockTwitchIrcServer twitch;
    mock::ChatterinoBadges chatterinoBadges;
    FfzBadges ffzBadges;
    SeventvBadges seventvBadges;
    HighlightController highlights;
    BttvEmotes bttvEmotes;
    FfzEmotes ffzEmotes;
    SeventvEmotes seventvEmotes;
    DisabledStreamerMode streamerMode;
};

}  // namespace

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
            << "Input " << test.input << " failed";
    }
}

class TestTwitchMessageBuilder : public ::testing::Test
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
        auto *privmsg =
            Communi::IrcPrivateMessage::fromData(test.input, nullptr);

        auto outputBadgeInfo =
            TwitchMessageBuilder::parseBadgeInfoTag(privmsg->tags());
        EXPECT_EQ(outputBadgeInfo, test.expectedBadgeInfo)
            << "Input for badgeInfo " << test.input << " failed";

        auto outputBadges =
            SharedMessageBuilder::parseBadgeTag(privmsg->tags());
        EXPECT_EQ(outputBadges, test.expectedBadges)
            << "Input for badges " << test.input << " failed";

        delete privmsg;
    }
}

TEST_F(TestTwitchMessageBuilder, ParseTwitchEmotes)
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
        auto *privmsg = static_cast<Communi::IrcPrivateMessage *>(
            Communi::IrcPrivateMessage::fromData(test.input, nullptr));
        QString originalMessage = privmsg->content();

        // TODO: Add tests with replies
        auto actualTwitchEmotes = TwitchMessageBuilder::parseTwitchEmotes(
            privmsg->tags(), originalMessage, 0);

        EXPECT_EQ(actualTwitchEmotes, test.expectedTwitchEmotes)
            << "Input for twitch emotes " << test.input << " failed";

        delete privmsg;
    }
}

TEST_F(TestTwitchMessageBuilder, ParseMessage)
{
    MockChannel channel("pajlada");

    struct TestCase {
        QByteArray input;
    };

    std::vector<TestCase> testCases{
        {
            // action /me message
            R"(@badge-info=subscriber/80;badges=broadcaster/1,subscriber/3072,partner/1;color=#CC44FF;display-name=pajlada;emote-only=1;emotes=25:0-4;first-msg=0;flags=;id=90ef1e46-8baa-4bf2-9c54-272f39d6fa11;mod=0;returning-chatter=0;room-id=11148817;subscriber=1;tmi-sent-ts=1662206235860;turbo=0;user-id=11148817;user-type= :pajlada!pajlada@pajlada.tmi.twitch.tv PRIVMSG #pajlada :ACTION Kappa)",
        },
        {
            R"(@badge-info=subscriber/17;badges=subscriber/12,no_audio/1;color=#EBA2C0;display-name=jammehcow;emote-only=1;emotes=25:0-4;first-msg=0;flags=;id=9c2dd916-5a6d-4c1f-9fe7-a081b62a9c6b;mod=0;returning-chatter=0;room-id=11148817;subscriber=1;tmi-sent-ts=1662201093248;turbo=0;user-id=82674227;user-type= :jammehcow!jammehcow@jammehcow.tmi.twitch.tv PRIVMSG #pajlada :Kappa)",
        },
        {
            R"(@badge-info=;badges=no_audio/1;color=#DAA520;display-name=Mm2PL;emote-only=1;emotes=1902:0-4;first-msg=0;flags=;id=9b1c3cb9-7817-47ea-add1-f9d4a9b4f846;mod=0;returning-chatter=0;room-id=11148817;subscriber=0;tmi-sent-ts=1662201095690;turbo=0;user-id=117691339;user-type= :mm2pl!mm2pl@mm2pl.tmi.twitch.tv PRIVMSG #pajlada :Keepo)",
        },
        {
            R"(@badge-info=;badges=no_audio/1;color=#DAA520;display-name=Mm2PL;emote-only=1;emotes=25:0-4/1902:6-10/305954156:12-19;first-msg=0;flags=;id=7be87072-bf24-4fa3-b3df-0ea6fa5f1474;mod=0;returning-chatter=0;room-id=11148817;subscriber=0;tmi-sent-ts=1662201102276;turbo=0;user-id=117691339;user-type= :mm2pl!mm2pl@mm2pl.tmi.twitch.tv PRIVMSG #pajlada :Kappa Keepo PogChamp)",
        },
        {
            R"(@badge-info=subscriber/80;badges=broadcaster/1,subscriber/3072,partner/1;color=#CC44FF;display-name=pajlada;emote-only=1;emotes=25:0-4,6-10;first-msg=0;flags=;id=f7516287-e5d1-43ca-974e-fe0cff84400b;mod=0;returning-chatter=0;room-id=11148817;subscriber=1;tmi-sent-ts=1662204375009;turbo=0;user-id=11148817;user-type= :pajlada!pajlada@pajlada.tmi.twitch.tv PRIVMSG #pajlada :Kappa Kappa)",
        },
        {
            R"(@badge-info=subscriber/80;badges=broadcaster/1,subscriber/3072,partner/1;color=#CC44FF;display-name=pajlada;emotes=25:0-4,8-12;first-msg=0;flags=;id=44f85d39-b5fb-475d-8555-f4244f2f7e82;mod=0;returning-chatter=0;room-id=11148817;subscriber=1;tmi-sent-ts=1662204423418;turbo=0;user-id=11148817;user-type= :pajlada!pajlada@pajlada.tmi.twitch.tv PRIVMSG #pajlada :Kappa 😂 Kappa)",
        },
        {
            // start out of range
            R"(@emotes=84608:9-10 :test!test@test.tmi.twitch.tv PRIVMSG #pajlada :foo bar)",
        },
        {
            // one character emote
            R"(@emotes=84608:0-0 :test!test@test.tmi.twitch.tv PRIVMSG #pajlada :foo bar)",
        },
        {
            // two character emote
            R"(@emotes=84609:0-1 :test!test@test.tmi.twitch.tv PRIVMSG #pajlada :foo bar)",
        },
        {
            // end out of range
            R"(@emotes=84608:0-15 :test!test@test.tmi.twitch.tv PRIVMSG #pajlada :foo bar)",
        },
        {
            // range bad (end character before start)
            R"(@emotes=84608:15-2 :test!test@test.tmi.twitch.tv PRIVMSG #pajlada :foo bar)",
        },
    };

    for (const auto &test : testCases)
    {
        auto *privmsg = dynamic_cast<Communi::IrcPrivateMessage *>(
            Communi::IrcPrivateMessage::fromData(test.input, nullptr));
        EXPECT_NE(privmsg, nullptr);

        QString originalMessage = privmsg->content();

        TwitchMessageBuilder builder(&channel, privmsg, MessageParseArgs{});

        auto msg = builder.build();
        EXPECT_NE(msg.get(), nullptr);

        delete privmsg;
    }
}

TEST_F(TestTwitchMessageBuilder, IgnoresReplace)
{
    struct TestCase {
        std::vector<IgnorePhrase> phrases;
        QString input;
        std::vector<TwitchEmoteOccurrence> twitchEmotes;
        QString expectedMessage;
        std::vector<TwitchEmoteOccurrence> expectedTwitchEmotes;
    };

    auto *twitchEmotes = this->mockApplication->getEmotes()->getTwitchEmotes();

    auto emoteAt = [&](int at, const QString &name) {
        return TwitchEmoteOccurrence{
            .start = at,
            .end = static_cast<int>(at + name.size() - 1),
            .ptr =
                twitchEmotes->getOrCreateEmote(EmoteId{name}, EmoteName{name}),
            .name = EmoteName{name},
        };
    };

    auto regularReplace = [](auto pattern, auto replace,
                             bool caseSensitive = true) {
        return IgnorePhrase(pattern, false, false, replace, caseSensitive);
    };
    auto regexReplace = [](auto pattern, auto regex,
                           bool caseSensitive = true) {
        return IgnorePhrase(pattern, true, false, regex, caseSensitive);
    };

    std::vector<TestCase> testCases{
        {
            {regularReplace("foo1", "baz1")},
            "foo1 Kappa",
            {emoteAt(4, "Kappa")},
            "baz1 Kappa",
            {emoteAt(4, "Kappa")},
        },
        {
            {regularReplace("foo1", "baz1", false)},
            "FoO1 Kappa",
            {emoteAt(4, "Kappa")},
            "baz1 Kappa",
            {emoteAt(4, "Kappa")},
        },
        {
            {regexReplace("f(o+)1", "baz1[\\1]")},
            "foo1 Kappa",
            {emoteAt(4, "Kappa")},
            "baz1[oo] Kappa",
            {emoteAt(8, "Kappa")},
        },

        {
            {regexReplace("f(o+)1", R"(baz1[\0][\1][\2])")},
            "foo1 Kappa",
            {emoteAt(4, "Kappa")},
            "baz1[\\0][oo][\\2] Kappa",
            {emoteAt(16, "Kappa")},
        },
        {
            {regexReplace("f(o+)(\\d+)", "baz1[\\1+\\2]")},
            "foo123 Kappa",
            {emoteAt(6, "Kappa")},
            "baz1[oo+123] Kappa",
            {emoteAt(12, "Kappa")},
        },
        {
            {regexReplace("(?<=foo)(\\d+)", "[\\1]")},
            "foo123 Kappa",
            {emoteAt(6, "Kappa")},
            "foo[123] Kappa",
            {emoteAt(8, "Kappa")},
        },
        {
            {regexReplace("a(?=a| )", "b")},
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa "
            "Kappa",
            {emoteAt(127, "Kappa")},
            "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
            "bbbb"
            "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb "
            "Kappa",
            {emoteAt(127, "Kappa")},
        },
        {
            {regexReplace("abc", "def", false)},
            "AbC Kappa",
            {emoteAt(3, "Kappa")},
            "def Kappa",
            {emoteAt(3, "Kappa")},
        },
        {
            {
                regexReplace("abc", "def", false),
                regularReplace("def", "ghi"),
            },
            "AbC Kappa",
            {emoteAt(3, "Kappa")},
            "ghi Kappa",
            {emoteAt(3, "Kappa")},
        },
        {
            {
                regexReplace("a(?=a| )", "b"),
                regexReplace("b(?=b| )", "c"),
            },
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa "
            "Kappa",
            {emoteAt(127, "Kappa")},
            "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
            "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc "
            "Kappa",
            {emoteAt(127, "Kappa")},
        },
    };

    for (const auto &test : testCases)
    {
        auto message = test.input;
        auto emotes = test.twitchEmotes;
        TwitchMessageBuilder::processIgnorePhrases(test.phrases, message,
                                                   emotes);

        EXPECT_EQ(message, test.expectedMessage)
            << "Message not equal for input '" << test.input
            << "' - expected: '" << test.expectedMessage << "' got: '"
            << message << "'";
        EXPECT_EQ(emotes, test.expectedTwitchEmotes)
            << "Twitch emotes not equal for input '" << test.input
            << "' and output '" << message << "'";
    }
}
