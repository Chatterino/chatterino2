// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/splits/InputHighlighter.hpp"

#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "controllers/spellcheck/SpellChecker.hpp"
#include "messages/Emote.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/EmoteController.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "providers/twitch/IrcMessageHandler.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "Test.hpp"

#include <QDebug>
#include <QString>
#include <QStringBuilder>

using namespace chatterino;
using namespace Qt::Literals;

namespace {

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication()
        : commandController(this->paths_)
    {
    }

    EmoteController *getEmotes() override
    {
        return &this->emotes;
    }

    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

    ITwitchIrcServer *getTwitch() override
    {
        return &this->twitch;
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

    CommandController *getCommands() override
    {
        return &this->commandController;
    }

    AccountController accounts;
    mock::EmoteController emotes;
    mock::MockTwitchIrcServer twitch;
    BttvEmotes bttvEmotes;
    FfzEmotes ffzEmotes;
    SeventvEmotes seventvEmotes;
    CommandController commandController;
};

std::pair<const EmoteName, EmotePtr> makeEmote(const QString &name)
{
    EmoteName emoteName{name};
    auto ptr = std::make_shared<Emote>(Emote{.name = emoteName});
    return {emoteName, ptr};
}

using EmoteMapPtr = std::shared_ptr<const EmoteMap>;

EmoteMapPtr makeEmotes(auto &&...emotes)
{
    auto map = std::make_shared<EmoteMap>();
    ((map->emplace(makeEmote(std::forward<decltype(emotes)>(emotes)))), ...);
    return map;
}

struct MockEmotes {
    EmoteMapPtr seventv;
    EmoteMapPtr bttv;
    EmoteMapPtr ffz;
    EmoteMapPtr twitchAccount;

    static MockEmotes channel()
    {
        return {
            .seventv = makeEmotes(u"7TVEmote"_s, u"PogChamp"_s),
            .bttv = makeEmotes(u"BTTVEmote"_s, u"Kappa"_s),
            .ffz = makeEmotes(u"FFZEmote"_s, u"Keepo"_s),
        };
    }

    static MockEmotes global()
    {
        return {
            .seventv = makeEmotes(u"7TVGlobal"_s),
            .bttv = makeEmotes(u"BTTVGlobal"_s),
            .ffz = makeEmotes(u"FFZGlobal"_s),
            .twitchAccount = makeEmotes(u"MyCoolTwitchEmote"_s),
        };
    }
};

std::shared_ptr<TwitchChannel> makeMockTwitchChannel(const QString &name)
{
    auto chan = std::make_shared<TwitchChannel>(name);
    auto mocks = MockEmotes::channel();
    chan->setSeventvEmotes(std::move(mocks.seventv));
    chan->setBttvEmotes(std::move(mocks.bttv));
    chan->setFfzEmotes(std::move(mocks.ffz));

    chan->addRecentChatter("UserChatter");
    chan->addRecentChatter("UserColor");
    chan->addRecentChatter("MyUser42");
    chan->addRecentChatter("123kappa123");

    return chan;
}

}  // namespace

class InputHighlighterTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        this->mockApplication = std::make_unique<MockApplication>();
        auto mocks = MockEmotes::global();
        this->mockApplication->seventvEmotes.setGlobalEmotes(mocks.seventv);
        this->mockApplication->bttvEmotes.setEmotes(mocks.bttv);
        this->mockApplication->ffzEmotes.setEmotes(mocks.ffz);
        this->mockApplication->getAccounts()->twitch.getCurrent()->setEmotes(
            mocks.twitchAccount);
        this->channel = makeMockTwitchChannel(u"twitchdev"_s);
        this->mockApplication->commandController.items.append(
            Command("/my-command", ""));
        this->mockApplication->commandController.items.append(
            Command("command space", ""));
        this->mockApplication->commandController.items.append(
            Command("command with more than one space", ""));
    }

    void TearDown() override
    {
        this->channel.reset();
        this->mockApplication.reset();
    }

    std::shared_ptr<TwitchChannel> channel;
    std::unique_ptr<MockApplication> mockApplication;
};

TEST_F(InputHighlighterTest, getSpellCheckedWords)
{
    struct Case {
        QString input;
        std::vector<QString> words;
    };

    std::vector<Case> cases{
        {.input = "", .words = {}},
        {.input = "word", .words = {"word"}},
        {.input = "word word", .words = {"word", "word"}},
        {.input = "   word word  ", .words = {"word", "word"}},
        {.input = "word?", .words = {"word"}},
        {.input = "word?word", .words = {"word", "word"}},
        // FIXME: should  be "word-word"
        {.input = "word-word", .words = {"word", "word"}},
        {
            .input = "channel emotes 7TVEmote a BTTVEmote b FFZEmote c",
            .words = {"channel", "emotes", "a", "b", "c"},
        },
        {
            .input = "global emotes 7TVGlobal a BTTVGlobal b FFZGlobal c "
                     "MyCoolTwitchEmote d",
            .words = {"global", "emotes", "a", "b", "c", "d"},
        },
        {
            .input = "/ban a user",
            .words = {"a", "user"},
        },
        {
            .input = "/my-command some text",
            .words = {"some", "text"},
        },
        {
            .input = "command space and some text",
            .words = {"and", "some", "text"},
        },
        {
            .input = "command with more than one space and some text",
            .words = {"and", "some", "text"},
        },
        {
            .input = "command with more than one and some text",
            .words =
                {
                    "command",
                    "with",
                    "more",
                    "than",
                    "one",
                    "and",
                    "some",
                    "text",
                },
        },
        {
            .input = "Hey, @userchatter, a 123kappa123 b MyUser42 c",
            .words = {"Hey", "a", "b", "c"},
        },
        {
            .input =
                "twitch.tv ignore "
                "https://wiki.chatterino.com/Help/#basic-troubleshooting links",
            .words = {"ignore", "links"},
        },
    };

    SpellChecker nullSpellChecker;
    InputHighlighter highlighter(nullSpellChecker, nullptr);
    highlighter.setChannel(this->channel);
    for (size_t i = 0; i < cases.size(); i++)
    {
        const auto &c = cases[i];
        auto got = highlighter.getSpellCheckedWords(c.input);
        ASSERT_EQ(got, c.words) << "index=" << i;
    }
}

TEST(InputHighlight, wordRegex)
{
    struct Case {
        QStringView input;
        std::vector<QStringView> words;
    };

    std::vector<Case> cases{
        {.input = u"", .words = {}},
        {.input = u"word", .words = {u"word"}},
        {.input = u"test/man", .words = {u"test", u"man"}},
        {.input = u"word?", .words = {u"word"}},
        {.input = u"word?word", .words = {u"word", u"word"}},
        {.input = u"sentence.", .words = {u"sentence"}},
        {.input = u"#hashtag", .words = {u"hashtag"}},
        {.input = u"inogre123numbers", .words = {}},
        {.input = u"under_score", .words = {}},
        {.input = u"äwördü", .words = {u"äwördü"}},
        {.input = u"abc!@foo#bar&(baz]",
         .words = {u"abc", u"foo", u"bar", u"baz"}},
        {.input = u"1234567,word/a123", .words = {u"word"}},
        {.input = u"'quotes\"", .words = {u"quotes"}},
    };

    auto re = inputhighlight::detail::wordRegex();
    for (size_t i = 0; i < cases.size(); i++)
    {
        const auto &c = cases[i];
        std::vector<QStringView> got;
        auto match = re.globalMatchView(c.input);
        while (match.hasNext())
        {
            got.emplace_back(match.next().capturedView());
        }
        ASSERT_EQ(got, c.words) << "index=" << i;
    }
}
