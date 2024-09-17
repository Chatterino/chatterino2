#include "messages/MessageBuilder.hpp"

#include "common/Literals.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "messages/Emote.hpp"
#include "messages/Message.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/Channel.hpp"
#include "mocks/ChatterinoBadges.hpp"
#include "mocks/DisabledStreamerMode.hpp"
#include "mocks/Logging.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "mocks/UserData.hpp"
#include "providers/ffz/FfzBadges.hpp"
#include "providers/seventv/SeventvBadges.hpp"
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

#include <set>
#include <unordered_map>
#include <vector>

using namespace chatterino;
using namespace literals;
using chatterino::mock::MockChannel;

namespace {

constexpr bool UPDATE_FIXTURES = false;

constexpr std::array IRC_FIXTURES{
    "action",  "emote-emoji", "emote",  "emotes",
    "emotes2", "rm-deleted",  "simple",
};

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication()
        : highlights(this->settings, &this->accounts)
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

    mock::EmptyLogging logging;
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
    TwitchBadges twitchBadges;
};

struct Fixture {
    QString category;
    QString name;
    QByteArray input;
    QJsonValue output;

    static QDir baseDir(const QString &category);
    static QString filePath(const QString &category, const QString &name);

    static bool verifyIntegrity(const QString &category, const auto &values);

    static Fixture read(QString category, QString name);

    void write(const QJsonValue &got) const;
    bool run(const QJsonValue &got) const;
};

bool compareJson(const QJsonValue &expected, const QJsonValue &got,
                 const QString &context)
{
    if (expected == got)
    {
        return true;
    }
    if (expected.type() != got.type())
    {
        qWarning() << context
                   << "- mismatching type - expected:" << expected.type()
                   << "got:" << got.type();
        return false;
    }
    switch (expected.type())
    {
        case QJsonValue::Array: {
            auto expArr = expected.toArray();
            auto gotArr = got.toArray();
            if (expArr.size() != gotArr.size())
            {
                qWarning() << context << "- Mismatching array size - expected:"
                           << expArr.size() << "got:" << gotArr.size();
                return false;
            }
            for (QJsonArray::size_type i = 0; i < expArr.size(); i++)
            {
                if (!compareJson(expArr[i], gotArr[i],
                                 context % '[' % QString::number(i) % ']'))
                {
                    return false;
                }
            }
        }
        break;  // unreachable
        case QJsonValue::Object: {
            auto expObj = expected.toObject();
            auto gotObj = got.toObject();
            if (expObj.size() != gotObj.size())
            {
                qWarning() << context << "- Mismatching object size - expected:"
                           << expObj.size() << "got:" << gotObj.size();
                return false;
            }
            for (auto it = expObj.constBegin(); it != expObj.constEnd(); it++)
            {
                if (!gotObj.contains(it.key()))
                {
                    qWarning() << context << "- Object doesn't contain key"
                               << it.key();
                    return false;
                }
                if (!compareJson(it.value(), gotObj[it.key()],
                                 context % '.' % it.key()))
                {
                    return false;
                }
            }
        }
        break;
        case QJsonValue::Null:
        case QJsonValue::Bool:
        case QJsonValue::Double:
        case QJsonValue::String:
        case QJsonValue::Undefined:
            break;
    }

    qWarning() << context << "- expected:" << expected << "got:" << got;
    return false;
}

QDir Fixture::baseDir(const QString &category)
{
    QDir fixtureDir(QStringLiteral(__FILE__));
    fixtureDir.cd("../../fixtures/MessageBuilder");
    fixtureDir.cd(category);
    return fixtureDir;
}

QString Fixture::filePath(const QString &category, const QString &name)
{
    return Fixture::baseDir(category).filePath(name);
}

bool Fixture::verifyIntegrity(const QString &category, const auto &values)
{
    auto files = Fixture::baseDir(category).entryList(QDir::NoDotAndDotDot |
                                                      QDir::Files);
    bool ok = true;
    if (static_cast<size_t>(files.size()) != values.size())
    {
        qWarning() << "Mismatching size!";
        ok = false;
    }

    // check that all files have some value (not the other way around)
    std::set<QString> valueSet;
    for (const auto &value : values)
    {
        valueSet.emplace(value);
    }
    for (const auto &file : files)
    {
        if (!file.endsWith(u".json"_s))
        {
            qWarning() << "Bad file:" << file;
            ok = false;
            continue;
        }
        if (!valueSet.contains(file.mid(0, file.length() - 5)))
        {
            qWarning() << file << "exists but isn't present in tests";
            ok = false;
        }
    }
    return ok;
}

Fixture Fixture::read(QString category, QString name)
{
    QFile file(Fixture::filePath(category, name));
    if (!file.open(QFile::ReadOnly))
    {
        throw std::runtime_error("Failed to open file");
    }
    auto content = file.readAll();
    file.close();
    const auto doc = QJsonDocument::fromJson(content).object();
    return {
        .category = std::move(category),
        .name = std::move(name),
        .input = doc["input"].toString().toUtf8(),
        .output = doc["output"],
    };
}

void Fixture::write(const QJsonValue &got) const
{
    QFile file(Fixture::filePath(this->category, this->name));
    if (!file.open(QFile::WriteOnly))
    {
        throw std::runtime_error("Failed to open file");
    }
    file.write(QJsonDocument{
        {
            {"input", QString::fromUtf8(this->input)},
            {"output", got},
        }}.toJson());
    file.close();
}

bool Fixture::run(const QJsonValue &got) const
{
    if (UPDATE_FIXTURES)
    {
        this->write(got);
        return true;
    }

    return compareJson(this->output, got, QStringLiteral("output"));
}

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
        };
    }
};

QT_WARNING_POP

std::shared_ptr<TwitchChannel> makeMockTwitchChannel(const QString &name)
{
    auto chan = std::make_shared<TwitchChannel>(name);
    auto mocks = MockEmotes::channel();
    chan->setSeventvEmotes(std::move(mocks.seventv));
    chan->setBttvEmotes(std::move(mocks.bttv));
    chan->setFfzEmotes(std::move(mocks.ffz));
    return chan;
}

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
        auto *privmsg = static_cast<Communi::IrcPrivateMessage *>(
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

TEST_F(TestMessageBuilder, ParseMessage)
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

        MessageBuilder builder(&channel, privmsg, MessageParseArgs{});

        auto msg = builder.build();
        EXPECT_NE(msg.get(), nullptr);

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

class TestMessageBuilderP : public ::testing::TestWithParam<const char *>
{
public:
    void SetUp() override
    {
        this->mockApplication = std::make_unique<MockApplication>();
        auto mocks = MockEmotes::global();
        this->mockApplication->seventvEmotes.setGlobalEmotes(mocks.seventv);
        this->mockApplication->bttvEmotes.setEmotes(mocks.bttv);
        this->mockApplication->ffzEmotes.setEmotes(mocks.ffz);
    }

    void TearDown() override
    {
        this->mockApplication.reset();
    }

    std::unique_ptr<MockApplication> mockApplication;
};

TEST_P(TestMessageBuilderP, Run)
{
    auto channel = makeMockTwitchChannel(u"pajlada"_s);
    const auto *param = std::remove_pointer_t<decltype(this)>::GetParam();

    auto fixture = Fixture::read(u"IRC"_s, param % QStringView(u".json"));
    auto *ircMessage = Communi::IrcMessage::fromData(fixture.input, nullptr);
    ASSERT_NE(ircMessage, nullptr);

    auto *privMsg = dynamic_cast<Communi::IrcPrivateMessage *>(ircMessage);
    ASSERT_NE(privMsg, nullptr);  // other types not yet supported
    MessageBuilder builder(channel.get(), privMsg, MessageParseArgs{});

    auto msg = builder.build();

    QJsonValue got = msg ? msg->toJson() : QJsonValue{};

    ASSERT_TRUE(fixture.run(got));

    delete ircMessage;
}

INSTANTIATE_TEST_SUITE_P(IrcMessage, TestMessageBuilderP,
                         testing::ValuesIn(IRC_FIXTURES));

TEST(TestMessageBuilderP, Integrity)
{
    ASSERT_TRUE(Fixture::verifyIntegrity(u"IRC"_s, IRC_FIXTURES));
    ASSERT_FALSE(UPDATE_FIXTURES);  // make sure fixtures are actually tested
}
