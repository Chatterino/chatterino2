#include "messages/MessageBuilder.hpp"

#include "common/Literals.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "controllers/sound/NullBackend.hpp"
#include "lib/Snapshot.hpp"
#include "messages/Emote.hpp"
#include "messages/Message.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/ChatterinoBadges.hpp"
#include "mocks/DisabledStreamerMode.hpp"
#include "mocks/Emotes.hpp"
#include "mocks/LinkResolver.hpp"
#include "mocks/Logging.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "mocks/UserData.hpp"
#include "providers/ffz/FfzBadges.hpp"
#include "providers/seventv/SeventvBadges.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/ChannelPointReward.hpp"
#include "providers/twitch/IrcMessageHandler.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Emotes.hpp"
#include "Test.hpp"
#include "util/IrcHelpers.hpp"

#include <IrcConnection>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <unordered_map>
#include <vector>

using namespace chatterino;
using namespace literals;

namespace {

/// Controls whether snapshots will be updated (true) or verified (false)
///
/// In CI, all snapshots must be verified, thus the integrity tests checks for
/// this constant.
///
/// When adding a test, start with `{ "input": "..." }` and set this to `true`
/// to generate an initial snapshot. Make sure to verify the output!
constexpr bool UPDATE_SNAPSHOTS = true;

const QString IRC_CATEGORY = u"MessageBuilder/IRC"_s;

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication()
        : highlights(this->settings, &this->accounts)
    {
    }

    MockApplication(const QString &settingsData)
        : mock::BaseApplication(settingsData)
        , highlights(this->settings, &this->accounts)
    {
    }

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

    ILogging *getChatLogger() override
    {
        return &this->logging;
    }

    TwitchBadges *getTwitchBadges() override
    {
        return &this->twitchBadges;
    }

    ILinkResolver *getLinkResolver() override
    {
        return &this->linkResolver;
    }

    ISoundController *getSound() override
    {
        return &this->sound;
    }

    mock::EmptyLogging logging;
    AccountController accounts;
    mock::Emotes emotes;
    mock::UserDataController userData;
    mock::MockTwitchIrcServer twitch;
    mock::ChatterinoBadges chatterinoBadges;
    FfzBadges ffzBadges;
    SeventvBadges seventvBadges;
    HighlightController highlights;
    BttvEmotes bttvEmotes;
    FfzEmotes ffzEmotes;
    SeventvEmotes seventvEmotes;
    TwitchBadges twitchBadges;
    mock::EmptyLinkResolver linkResolver;
    NullBackend sound;
};

std::pair<const EmoteName, EmotePtr> makeEmote(Emote &&emote)
{
    auto ptr = std::make_shared<Emote>(std::move(emote));
    ptr->homePage = {u"https://chatterino.com/" % ptr->name.string};
    ptr->tooltip = {ptr->name.string % u" Tooltip"_s};
    ptr->author = {u"Chatterino"_s};
    ptr->images = {
        Url{u"https://chatterino.com/" % ptr->name.string % u".png"}};
    return {ptr->name, ptr};
}

using EmoteMapPtr = std::shared_ptr<const EmoteMap>;

EmoteMapPtr makeEmotes(auto &&...emotes)
{
    auto map = std::make_shared<EmoteMap>();
    ((map->emplace(makeEmote(std::forward<decltype(emotes)>(emotes)))), ...);
    return map;
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wmissing-field-initializers")

struct MockEmotes {
    EmoteMapPtr seventv;
    EmoteMapPtr bttv;
    EmoteMapPtr ffz;
    EmoteMapPtr twitchAccount;

    static MockEmotes channel()
    {
        return {
            .seventv = makeEmotes(
                Emote{
                    .name = {u"7TVEmote"_s},
                    .id = {u"1"_s},
                },
                Emote{
                    .name = {u"7TVEmote0w"_s},
                    .zeroWidth = true,
                    .id = {u"2"_s},
                    .baseName = EmoteName{u"ZeroWidth"_s},
                },
                Emote{
                    .name = {u"PogChamp"_s},
                    .id = {u"3"_s},
                }),
            .bttv = makeEmotes(
                Emote{
                    .name = {u"BTTVEmote"_s},
                },
                Emote{
                    .name = {u"Kappa"_s},
                }),
            .ffz = makeEmotes(
                Emote{
                    .name = {u"FFZEmote"_s},
                },
                Emote{
                    .name = {u"Keepo"_s},
                }),
        };
    }

    static MockEmotes global()
    {
        return {
            .seventv = makeEmotes(Emote{
                .name = {u"7TVGlobal"_s},
                .id = {u"G1"_s},
            }),
            .bttv = makeEmotes(Emote{
                .name = {u"BTTVGlobal"_s},
            }),
            .ffz = makeEmotes(Emote{
                .name = {u"FFZGlobal"_s},
            }),
            .twitchAccount = makeEmotes(Emote{
                .name = {u"MyCoolTwitchEmote"_s},
                .id = {u"5678"_s},
            }),
        };
    }
};

const QByteArray CHEERMOTE_JSON{R"({
    "prefix": "Cheer",
    "tiers": [
    {
        "min_bits": 1,
        "id": "1",
        "color": "#979797",
        "images": {
            "dark": {
                "animated": {
                    "1": "https://chatterino.com/bits/1.gif",
                    "2": "https://chatterino.com/bits/2.gif",
                    "4": "https://chatterino.com/bits/4.gif"
                },
                "static": {
                    "1": "https://chatterino.com/bits/1.png",
                    "2": "https://chatterino.com/bits/2.png",
                    "4": "https://chatterino.com/bits/4.png"
                }
            }
        },
        "can_cheer": true,
        "show_in_bits_card": true
    },
    {
        "min_bits": 100,
        "id": "100",
        "color": "#9c3ee8",
        "images": {
            "dark": {
                "animated": {
                    "1": "https://chatterino.com/bits/1.gif",
                    "2": "https://chatterino.com/bits/2.gif",
                    "4": "https://chatterino.com/bits/4.gif"
                },
                "static": {
                    "1": "https://chatterino.com/bits/1.png",
                    "2": "https://chatterino.com/bits/2.png",
                    "4": "https://chatterino.com/bits/4.png"
                }
            }
        },
        "can_cheer": true,
        "show_in_bits_card": true
    }
    ],
    "type": "global_first_party",
    "order": 1,
    "last_updated": "2018-05-22T00:06:04Z",
    "is_charitable": false
})"_ba};

const QByteArray LOCAL_BADGE_JSON{R"({
    "data": [
        {
            "set_id": "subscriber",
            "versions": [
                {
                "click_url": null,
                "description": "Subscriber",
                "id": "3072",
                "image_url_1x": "https://chatterino.com/tb-1",
                "image_url_2x": "https://chatterino.com/tb-2",
                "image_url_4x": "https://chatterino.com/tb-3",
                "title": "Subscriber"
                }
            ]
        }
    ]
})"_ba};

const QByteArray SETTINGS_DEFAULT{"{}"_ba};

std::shared_ptr<TwitchChannel> makeMockTwitchChannel(
    const QString &name, const testlib::Snapshot &snapshot)
{
    auto chan = std::make_shared<TwitchChannel>(name);
    auto mocks = MockEmotes::channel();
    chan->setSeventvEmotes(std::move(mocks.seventv));
    chan->setBttvEmotes(std::move(mocks.bttv));
    chan->setFfzEmotes(std::move(mocks.ffz));

    QJsonObject defaultImage{
        {u"url_1x"_s, u"https://chatterino.com/reward1x.png"_s},
        {u"url_2x"_s, u"https://chatterino.com/reward2x.png"_s},
        {u"url_4x"_s, u"https://chatterino.com/reward4x.png"_s},
    };
    chan->addKnownChannelPointReward({{
        {u"channel_id"_s, u"11148817"_s},
        {u"id"_s, u"unused"_s},
        {u"reward"_s,
         {{
             {u"channel_id"_s, u"11148817"_s},
             {u"cost"_s, 1},
             {u"id"_s, u"31a2344e-0fce-4229-9453-fb2e8b6dd02c"_s},
             {u"is_user_input_required"_s, true},
             {u"title"_s, u"my reward"_s},
             {u"image"_s, defaultImage},
         }}},
    }});
    chan->addKnownChannelPointReward({{
        {u"channel_id"_s, u"11148817"_s},
        {u"id"_s, u"unused"_s},
        {u"reward"_s,
         {{
             {u"channel_id"_s, u"11148817"_s},
             {u"cost"_s, 1},
             {u"id"_s, u"dc8d1dac-256e-42b9-b7ba-40b32e5294e2"_s},
             {u"is_user_input_required"_s, false},
             {u"title"_s, u"test"_s},
             {u"image"_s, defaultImage},
         }}},
    }});
    chan->addKnownChannelPointReward({{
        {u"channel_id"_s, u"11148817"_s},
        {u"id"_s, u"unused"_s},
        {u"reward"_s,
         {{
             {u"channel_id"_s, u"11148817"_s},
             {u"cost"_s, 1},
             {u"default_bits_cost"_s, 2},
             {u"bits_cost"_s, 0},
             {u"pricing_type"_s, u"BITS"_s},
             {u"reward_type"_s, u"CELEBRATION"_s},
             {u"is_user_input_required"_s, false},
             {u"title"_s, u"BitReward"_s},
             {u"image"_s, defaultImage},
         }}},
        {u"redemption_metadata"_s,
         QJsonObject{
             {u"celebration_emote_metadata"_s,
              QJsonObject{
                  {u"emote"_s,
                   {{
                       {u"id"_s, u"42"_s},
                       {u"token"_s, u"MyBitsEmote"_s},
                   }}},
              }},
         }},
    }});

    chan->setUserColor("UserColor", {1, 2, 3, 4});
    chan->setUserColor("UserColor2", {5, 6, 7, 8});
    chan->addRecentChatter("UserChatter");
    chan->addRecentChatter("UserColor");

    chan->setCheerEmoteSets({
        HelixCheermoteSet{QJsonDocument::fromJson(CHEERMOTE_JSON).object()},
    });

    chan->setFfzChannelBadges({{u"123456"_s, {3, 4}}});

    chan->addTwitchBadgeSets(HelixChannelBadges{
        QJsonDocument::fromJson(LOCAL_BADGE_JSON).object(),
    });

    if (snapshot.param("ffzCustomVipBadge").toBool())
    {
        chan->setFfzCustomVipBadge(std::make_shared<Emote>(Emote{
            .name = {},
            .images = {Url{"https://chatterino.com/ffz-vip1x.png"}},
            .tooltip = {"VIP"},
            .homePage = {},
        }));
    }
    if (snapshot.param("ffzCustomModBadge").toBool())
    {
        chan->setFfzCustomModBadge(std::make_shared<Emote>(Emote{
            .name = {},
            .images = {Url{"https://chatterino.com/ffz-mod1x.png"}},
            .tooltip = {"Moderator"},
            .homePage = {},
        }));
    }

    return chan;
}

QT_WARNING_POP

}  // namespace

TEST(MessageBuilder, CommaSeparatedListTagParsing)
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

class TestMessageBuilder : public ::testing::Test
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

TEST(MessageBuilder, BadgeInfoParsing)
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
            MessageBuilder::parseBadgeInfoTag(privmsg->tags());
        EXPECT_EQ(outputBadgeInfo, test.expectedBadgeInfo)
            << "Input for badgeInfo " << test.input << " failed";

        auto outputBadges = MessageBuilder::parseBadgeTag(privmsg->tags());
        EXPECT_EQ(outputBadges, test.expectedBadges)
            << "Input for badges " << test.input << " failed";

        delete privmsg;
    }
}

TEST_F(TestMessageBuilder, ParseTwitchEmotes)
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
        QString originalMessage = privmsg->content();

        // TODO: Add tests with replies
        auto actualTwitchEmotes = MessageBuilder::parseTwitchEmotes(
            privmsg->tags(), originalMessage, 0);

        EXPECT_EQ(actualTwitchEmotes, test.expectedTwitchEmotes)
            << "Input for twitch emotes " << test.input << " failed";

        delete privmsg;
    }
}

TEST_F(TestMessageBuilder, IgnoresReplace)
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
        MessageBuilder::processIgnorePhrases(test.phrases, message, emotes);

        EXPECT_EQ(message, test.expectedMessage)
            << "Message not equal for input '" << test.input
            << "' - expected: '" << test.expectedMessage << "' got: '"
            << message << "'";
        EXPECT_EQ(emotes, test.expectedTwitchEmotes)
            << "Twitch emotes not equal for input '" << test.input
            << "' and output '" << message << "'";
    }
}

class TestMessageBuilderP : public ::testing::TestWithParam<QString>
{
public:
    void SetUp() override
    {
        auto param = TestMessageBuilderP::GetParam();
        this->snapshot = testlib::Snapshot::read(IRC_CATEGORY, param);

        this->mockApplication =
            std::make_unique<MockApplication>(QString::fromUtf8(
                this->snapshot->mergedSettings(SETTINGS_DEFAULT)));
        auto mocks = MockEmotes::global();
        this->mockApplication->seventvEmotes.setGlobalEmotes(mocks.seventv);
        this->mockApplication->bttvEmotes.setEmotes(mocks.bttv);
        this->mockApplication->ffzEmotes.setEmotes(mocks.ffz);
        this->mockApplication->getAccounts()->twitch.getCurrent()->setEmotes(
            mocks.twitchAccount);
        this->mockApplication->getUserData()->setUserColor(u"117691339"_s,
                                                           u"#DAA521"_s);

        this->mockApplication->getAccounts()
            ->twitch.getCurrent()
            ->blockUserLocally(u"12345"_s);

        auto makeBadge = [](QStringView platform) {
            return std::make_shared<Emote>(Emote{
                .name = {},
                .images = {Url{u"https://chatterino.com/" % platform %
                               u".png"}},
                .tooltip = {platform % u" badge"},
                .homePage = {},
                .zeroWidth = false,
                .id = {},
                .author = {},
                .baseName = {},
            });
        };

        // Chatterino
        this->mockApplication->chatterinoBadges.setBadge(
            {u"123456"_s}, makeBadge(u"Chatterino"));

        // FFZ
        this->mockApplication->ffzBadges.registerBadge(
            1, {.emote = makeBadge(u"FFZ1"), .color = {9, 10, 11, 12}});
        this->mockApplication->ffzBadges.registerBadge(
            2, {.emote = makeBadge(u"FFZ2"), .color = {13, 14, 15, 16}});
        this->mockApplication->ffzBadges.registerBadge(
            3, {.emote = makeBadge(u"FFZ2"), .color = {17, 18, 19, 20}});
        this->mockApplication->ffzBadges.registerBadge(
            4, {.emote = makeBadge(u"FFZ2"), .color = {21, 22, 23, 24}});
        this->mockApplication->getFfzBadges()->assignBadgeToUser({u"123456"_s},
                                                                 1);
        this->mockApplication->getFfzBadges()->assignBadgeToUser({u"123456"_s},
                                                                 2);

        // 7TV
        this->mockApplication->getSeventvBadges()->registerBadge({
            {u"id"_s, u"1"_s},
            {u"tooltip"_s, u"7TV badge"_s},
            {
                u"host"_s,
                {{
                    {u"url"_s, u"//chatterino.com/7tv/"_s},
                    {u"files"_s,
                     QJsonArray{
                         {{
                             {u"name"_s, u"1x"_s},
                             {u"format"_s, u"WEBP"_s},
                             {u"width"_s, 16},
                         }},
                     }},
                }},
            },
        });
        this->mockApplication->getSeventvBadges()->assignBadgeToUser(
            u"1"_s, {u"123456"_s});

        // Twitch
        this->mockApplication->getTwitchBadges()->loadLocalBadges();

        this->twitchdevChannel = std::make_shared<TwitchChannel>("twitchdev");
        this->twitchdevChannel->setRoomId("141981764");
        this->mockApplication->twitch.mockChannels.emplace(
            "twitchdev", this->twitchdevChannel);
    }

    void TearDown() override
    {
        this->twitchdevChannel.reset();
        this->mockApplication.reset();
        this->snapshot.reset();
    }

    std::shared_ptr<TwitchChannel> twitchdevChannel;
    std::unique_ptr<MockApplication> mockApplication;
    std::unique_ptr<testlib::Snapshot> snapshot;
};

/// This tests the process of parsing IRC messages and emitting `MessagePtr`s.
///
/// Even though it's in the message builder category, this uses
/// `IrcMesssageHandler` to ensure the correct (or: "real") arguments to build
/// messages.
///
/// Tests are contained in `tests/snapshots/MessageBuilder/IRC`. Fixtures
/// consist of an object with the keys `input`, `output`, `settings` (optional),
/// and `params` (optional).
///
/// `UPDATE_SNAPSHOTS` (top) controls whether the `output` will be generated or
/// checked.
///
/// `params` is an optional object with the following keys:
/// - `prevMessages`: An array of past messages (used for replies)
/// - `findAllUsernames`: A boolean controlling the equally named setting
///   (default: false)
TEST_P(TestMessageBuilderP, Run)
{
    auto channel = makeMockTwitchChannel(u"pajlada"_s, *snapshot);

    std::vector<MessagePtr> prevMessages;

    for (auto prevInput : snapshot->param("prevMessages").toArray())
    {
        auto *ircMessage = Communi::IrcMessage::fromData(
            prevInput.toString().toUtf8(), nullptr);
        ASSERT_NE(ircMessage, nullptr);
        auto builtMessages = IrcMessageHandler::parseMessageWithReply(
            channel.get(), ircMessage, prevMessages);
        for (const auto &builtMessage : builtMessages)
        {
            prevMessages.emplace_back(builtMessage);
        }
        delete ircMessage;
    }

    auto *ircMessage =
        Communi::IrcMessage::fromData(snapshot->inputUtf8(), nullptr);
    ASSERT_NE(ircMessage, nullptr);

    auto builtMessages = IrcMessageHandler::parseMessageWithReply(
        channel.get(), ircMessage, prevMessages);

    QJsonArray got;
    for (const auto &msg : builtMessages)
    {
        got.append(msg->toJson());
    }

    delete ircMessage;

    ASSERT_TRUE(snapshot->run(got, UPDATE_SNAPSHOTS));
}

INSTANTIATE_TEST_SUITE_P(
    IrcMessage, TestMessageBuilderP,
    testing::ValuesIn(testlib::Snapshot::discover(IRC_CATEGORY)));

TEST(TestMessageBuilderP, Integrity)
{
    ASSERT_FALSE(UPDATE_SNAPSHOTS);  // make sure fixtures are actually tested
}
