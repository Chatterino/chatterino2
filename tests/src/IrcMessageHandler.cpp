#include "providers/twitch/IrcMessageHandler.hpp"

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
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Emotes.hpp"
#include "Test.hpp"
#include "util/IrcHelpers.hpp"
#include "util/VectorMessageSink.hpp"

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
constexpr bool UPDATE_SNAPSHOTS = false;

const QString IRC_CATEGORY = u"IrcMessageHandler"_s;

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

    static MockEmotes twitchdev()
    {
        return {
            .seventv = makeEmotes(Emote{
                .name = {u"7TVTwitchDev"_s},
                .id = {u"t5"_s},
            }),
            .bttv = makeEmotes(Emote{
                .name = {u"BTTVTwitchDev"_s},
            }),
            .ffz = makeEmotes(Emote{
                .name = {u"FFZTwitchDev"_s},
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

class TestIrcMessageHandlerP : public ::testing::TestWithParam<QString>
{
public:
    void SetUp() override
    {
        auto param = TestIrcMessageHandlerP::GetParam();
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

        auto tdMocks = MockEmotes::twitchdev();
        this->twitchdevChannel->setSeventvEmotes(std::move(tdMocks.seventv));
        this->twitchdevChannel->setBttvEmotes(std::move(tdMocks.bttv));
        this->twitchdevChannel->setFfzEmotes(std::move(tdMocks.ffz));

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
/// Tests are contained in `tests/snapshots/IrcMessageHandler`. Fixtures
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
/// - `nAdditional`: Include n additional built messages (from `prevMessages`)
TEST_P(TestIrcMessageHandlerP, Run)
{
    auto channel = makeMockTwitchChannel(u"pajlada"_s, *snapshot);

    VectorMessageSink sink;

    for (auto prevInput : snapshot->param("prevMessages").toArray())
    {
        auto *ircMessage = Communi::IrcMessage::fromData(
            prevInput.toString().toUtf8(), nullptr);
        ASSERT_NE(ircMessage, nullptr);
        IrcMessageHandler::parseMessageInto(ircMessage, sink, channel.get());
        delete ircMessage;
    }

    auto *ircMessage =
        Communi::IrcMessage::fromData(snapshot->inputUtf8(), nullptr);
    ASSERT_NE(ircMessage, nullptr);

    auto nAdditionalMessages = snapshot->param("nAdditional").toInt(0);
    ASSERT_GE(sink.messages().size(), nAdditionalMessages);

    auto firstAddedMsg = sink.messages().size() - nAdditionalMessages;
    IrcMessageHandler::parseMessageInto(ircMessage, sink, channel.get());

    QJsonArray got;
    for (auto i = firstAddedMsg; i < sink.messages().size(); i++)
    {
        got.append(sink.messages()[i]->toJson());
    }

    delete ircMessage;

    ASSERT_TRUE(snapshot->run(got, UPDATE_SNAPSHOTS))
        << "Snapshot " << snapshot->name() << " failed. Expected JSON to be\n"
        << QJsonDocument(snapshot->output().toArray()).toJson() << "\nbut got\n"
        << QJsonDocument(got).toJson() << "\ninstead.";
}

INSTANTIATE_TEST_SUITE_P(
    IrcMessage, TestIrcMessageHandlerP,
    testing::ValuesIn(testlib::Snapshot::discover(IRC_CATEGORY)));

TEST(TestIrcMessageHandlerP, Integrity)
{
    ASSERT_FALSE(UPDATE_SNAPSHOTS);  // make sure fixtures are actually tested
}
