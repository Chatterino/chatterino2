#include "Application.hpp"
#include "common/Aliases.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/completion/strategies/ClassicEmoteStrategy.hpp"
#include "controllers/completion/strategies/ClassicUserStrategy.hpp"
#include "controllers/completion/strategies/SmartEmoteStrategy.hpp"
#include "controllers/completion/strategies/Strategy.hpp"
#include "messages/Emote.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/Channel.hpp"
#include "mocks/Emotes.hpp"
#include "mocks/Helix.hpp"
#include "mocks/Logging.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "Test.hpp"
#include "widgets/splits/InputCompletionPopup.hpp"

#include <QDir>
#include <QFile>
#include <QModelIndex>
#include <QString>
#include <QTemporaryDir>

#include <span>

using namespace chatterino;
using chatterino::mock::MockChannel;

namespace {

using namespace chatterino::completion;
using ::testing::Exactly;

class MockApplication : public mock::BaseApplication
{
public:
    explicit MockApplication(const QString &settingsData)
        : BaseApplication(settingsData)
    {
    }

    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

    ITwitchIrcServer *getTwitch() override
    {
        return &this->twitch;
    }

    IEmotes *getEmotes() override
    {
        return &this->emotes;
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

    mock::EmptyLogging logging;
    AccountController accounts;
    mock::MockTwitchIrcServer twitch;
    mock::Emotes emotes;
    BttvEmotes bttvEmotes;
    FfzEmotes ffzEmotes;
    SeventvEmotes seventvEmotes;
};

void containsRoughly(std::span<EmoteItem> span, const std::set<QString> &values)
{
    for (const auto &v : values)
    {
        bool found = false;
        for (const auto &actualValue : span)
        {
            if (actualValue.displayName == v)
            {
                found = true;
                break;
            }
        }

        ASSERT_TRUE(found) << v << " was not found in the span";
    }
}

[[nodiscard]] bool allEmoji(std::span<EmoteItem> span)
{
    return std::ranges::all_of(span, [](const auto &it) {
        return it.isEmoji && it.providerName == u"Emoji";
    });
}

EmotePtr namedEmote(const EmoteName &name)
{
    return std::shared_ptr<Emote>(new Emote{
        .name{name},
        .images{},
        .tooltip{},
        .zeroWidth{},
        .id{},
        .author{},
    });
}

void addEmote(EmoteMap &map, const QString &name)
{
    EmoteName eName{.string{name}};
    map.insert(std::pair<EmoteName, EmotePtr>(eName, namedEmote(eName)));
}

const QString DEFAULT_SETTINGS = R"!(
{
    "accounts": {
        "uid117166826": {
            "username": "testaccount_420",
            "userID": "117166826",
            "clientID": "abc",
            "oauthToken": "def"
        },
        "current": "testaccount_420"
    }
})!";

}  // namespace

class InputCompletionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize helix client
        this->mockHelix = std::make_unique<mock::Helix>();
        initializeHelix(this->mockHelix.get());
        EXPECT_CALL(*this->mockHelix, loadBlocks).Times(Exactly(1));
        EXPECT_CALL(*this->mockHelix, update).Times(Exactly(1));

        this->mockApplication =
            std::make_unique<MockApplication>(DEFAULT_SETTINGS);

        this->mockApplication->accounts.load();

        this->channelPtr = std::make_shared<MockChannel>("icelys");

        this->initializeEmotes();
    }

    void TearDown() override
    {
        this->channelPtr.reset();
        this->mockHelix.reset();
        this->mockApplication.reset();
    }

    std::unique_ptr<MockApplication> mockApplication;
    std::unique_ptr<mock::Helix> mockHelix;

    ChannelPtr channelPtr;

private:
    void initializeEmotes()
    {
        auto bttvEmotes = std::make_shared<EmoteMap>();
        addEmote(*bttvEmotes, "FeelsGoodMan");
        addEmote(*bttvEmotes, "FeelsBadMan");
        addEmote(*bttvEmotes, "FeelsBirthdayMan");
        addEmote(*bttvEmotes, "Aware");
        addEmote(*bttvEmotes, "Clueless");
        addEmote(*bttvEmotes, "SaltyCorn");
        addEmote(*bttvEmotes, ":)");
        addEmote(*bttvEmotes, ":-)");
        addEmote(*bttvEmotes, "B-)");
        addEmote(*bttvEmotes, "Clap");
        addEmote(*bttvEmotes, ":tf:");
        this->mockApplication->bttvEmotes.setEmotes(std::move(bttvEmotes));

        auto ffzEmotes = std::make_shared<EmoteMap>();
        addEmote(*ffzEmotes, "LilZ");
        addEmote(*ffzEmotes, "ManChicken");
        addEmote(*ffzEmotes, "CatBag");
        this->mockApplication->ffzEmotes.setEmotes(std::move(ffzEmotes));

        auto seventvEmotes = std::make_shared<EmoteMap>();
        addEmote(*seventvEmotes, "Clap");
        addEmote(*seventvEmotes, "Clap2");
        addEmote(*seventvEmotes, "pajaW");
        addEmote(*seventvEmotes, "PAJAW");
        this->mockApplication->seventvEmotes.setGlobalEmotes(
            std::move(seventvEmotes));
    }

protected:
    template <typename T>
    auto queryEmoteCompletion(const QString &fullQuery)
    {
        EmoteSource source(this->channelPtr.get(), std::make_unique<T>());
        source.update(fullQuery);

        std::vector<EmoteItem> out(source.output());
        return out;
    }

    template <typename T>
    auto queryTabCompletion(const QString &fullQuery, bool isFirstWord)
    {
        EmoteSource source(this->channelPtr.get(), std::make_unique<T>());
        source.update(fullQuery);

        QStringList m;
        source.addToStringList(m, 0, isFirstWord);
        return m;
    }

    auto queryClassicEmoteCompletion(const QString &fullQuery)
    {
        return queryEmoteCompletion<ClassicEmoteStrategy>(fullQuery);
    }

    auto queryClassicTabCompletion(const QString &fullQuery, bool isFirstWord)
    {
        return queryTabCompletion<ClassicTabEmoteStrategy>(fullQuery,
                                                           isFirstWord);
    }

    auto querySmartEmoteCompletion(const QString &fullQuery)
    {
        return queryEmoteCompletion<SmartEmoteStrategy>(fullQuery);
    }

    auto querySmartTabCompletion(const QString &fullQuery, bool isFirstWord)
    {
        return queryTabCompletion<SmartTabEmoteStrategy>(fullQuery,
                                                         isFirstWord);
    }
};

TEST_F(InputCompletionTest, ClassicEmoteNameFiltering)
{
    // The completion doesn't guarantee an ordering for a specific category of emotes.
    // This tests a specific implementation of the underlying std::unordered_map,
    // so depending on the standard library used when compiling, this might yield
    // different results.

    auto completion = queryClassicEmoteCompletion(":feels");
    ASSERT_EQ(completion.size(), 3);
    // all these matches are BTTV global emotes
    // these are in no specific order
    containsRoughly(completion,
                    {"FeelsBirthdayMan", "FeelsBadMan", "FeelsGoodMan"});

    completion = queryClassicEmoteCompletion(":)");
    ASSERT_EQ(completion.size(), 3);
    ASSERT_EQ(completion[0].displayName, ":)");  // Exact match with : prefix
    containsRoughly({completion.begin() + 1, 2}, {":-)", "B-)"});

    completion = queryClassicEmoteCompletion(":cat");
    ASSERT_TRUE(completion.size() >= 2);
    // emoji exact match comes first
    ASSERT_EQ(completion[0].displayName, "cat");
    // FFZ emote is prioritized over any other matching emojis
    ASSERT_EQ(completion[1].displayName, "CatBag");
}

TEST_F(InputCompletionTest, ClassicEmoteExactNameMatching)
{
    auto completion = queryClassicEmoteCompletion(":cat");
    ASSERT_TRUE(completion.size() >= 2);
    // emoji exact match comes first
    ASSERT_EQ(completion[0].displayName, "cat");
    // FFZ emote is prioritized over any other matching emojis
    ASSERT_EQ(completion[1].displayName, "CatBag");

    // not exactly "salt", SaltyCorn BTTV emote comes first
    completion = queryClassicEmoteCompletion(":sal");
    ASSERT_TRUE(completion.size() >= 3);
    ASSERT_EQ(completion[0].displayName, "SaltyCorn");
    ASSERT_EQ(completion[1].displayName, "green_salad");
    ASSERT_EQ(completion[2].displayName, "salt");

    // exactly "salt", emoji comes first
    completion = queryClassicEmoteCompletion(":salt");
    ASSERT_TRUE(completion.size() >= 2);
    ASSERT_EQ(completion[0].displayName, "salt");
    ASSERT_EQ(completion[1].displayName, "SaltyCorn");
}

TEST_F(InputCompletionTest, ClassicEmoteProviderOrdering)
{
    auto completion = queryClassicEmoteCompletion(":clap");
    // Current implementation leads to the exact first match being ignored when
    // checking for exact matches. This is probably not intended behavior but
    // this test is just verifying that the implementation stays the same.
    //
    // Initial ordering after filtering all available emotes:
    //  1. Clap - BTTV
    //  2. Clap - 7TV
    //  3. Clap2 - 7TV
    //  4. clapper - Emoji
    //  5. clap - Emoji
    //
    // The 'exact match' starts looking at the second element and ends up swapping
    // #2 with #1 despite #1 already being an exact match.
    ASSERT_TRUE(completion.size() >= 5);
    ASSERT_EQ(completion[0].displayName, "Clap");
    ASSERT_EQ(completion[0].providerName, "Global 7TV");
    ASSERT_EQ(completion[1].displayName, "Clap");
    ASSERT_EQ(completion[1].providerName, "Global BetterTTV");
    ASSERT_EQ(completion[2].displayName, "Clap2");
    ASSERT_EQ(completion[2].providerName, "Global 7TV");
    ASSERT_EQ(completion[3].displayName, "clapper");
    ASSERT_EQ(completion[3].providerName, "Emoji");
    ASSERT_EQ(completion[4].displayName, "clap");
    ASSERT_EQ(completion[4].providerName, "Emoji");
}

TEST_F(InputCompletionTest, ClassicEmoteCase)
{
    auto completion = queryClassicEmoteCompletion(":pajaw");
    ASSERT_EQ(completion.size(), 2);
    // there's no order here
    containsRoughly(completion, {"pajaW", "PAJAW"});

    completion = queryClassicEmoteCompletion(":PA");
    ASSERT_GT(completion.size(), 3);
    containsRoughly({completion.begin(), 2}, {"pajaW", "PAJAW"});
    containsRoughly({completion.begin() + 2, completion.end()}, {"parking"});
    ASSERT_TRUE(allEmoji({completion.begin() + 2, completion.end()}));

    completion = queryClassicEmoteCompletion(":Pajaw");
    ASSERT_EQ(completion.size(), 2);
    containsRoughly(completion, {"pajaW", "PAJAW"});

    completion = queryClassicEmoteCompletion(":NOTHING");
    ASSERT_EQ(completion.size(), 0);

    completion = queryClassicEmoteCompletion(":nothing");
    ASSERT_EQ(completion.size(), 0);
}

TEST_F(InputCompletionTest, ClassicTabCompletionEmote)
{
    auto completion = queryClassicTabCompletion(":feels", false);
    ASSERT_EQ(completion.size(), 0);  // : prefix matters here

    // no : prefix defaults to emote completion
    completion = queryClassicTabCompletion("feels", false);
    ASSERT_EQ(completion.size(), 3);
    // note: different order from : menu
    ASSERT_EQ(completion[0], "FeelsBadMan ");
    ASSERT_EQ(completion[1], "FeelsBirthdayMan ");
    ASSERT_EQ(completion[2], "FeelsGoodMan ");

    // no : prefix, emote completion. Duplicate Clap should be removed
    completion = queryClassicTabCompletion("cla", false);
    ASSERT_EQ(completion.size(), 2);
    ASSERT_EQ(completion[0], "Clap ");
    ASSERT_EQ(completion[1], "Clap2 ");

    completion = queryClassicTabCompletion("peepoHappy", false);
    ASSERT_EQ(completion.size(), 0);  // no peepoHappy emote

    completion = queryClassicTabCompletion("Aware", false);
    ASSERT_EQ(completion.size(), 1);
    ASSERT_EQ(completion[0], "Aware ");  // trailing space added
}

TEST_F(InputCompletionTest, ClassicTabCompletionEmoji)
{
    auto completion = queryClassicTabCompletion(":tf", false);
    ASSERT_EQ(completion.size(), 1);
    ASSERT_EQ(completion[0], ":tf: ");

    completion = queryClassicTabCompletion(":)", false);
    ASSERT_EQ(completion.size(), 1);
    ASSERT_EQ(completion[0], ":) ");

    completion = queryClassicTabCompletion(":cla", false);
    ASSERT_EQ(completion.size(), 8);
    ASSERT_EQ(completion[0], ":clap: ");
    ASSERT_EQ(completion[1], ":clap_tone1: ");
    ASSERT_EQ(completion[2], ":clap_tone2: ");
    ASSERT_EQ(completion[3], ":clap_tone3: ");
    ASSERT_EQ(completion[4], ":clap_tone4: ");
    ASSERT_EQ(completion[5], ":clap_tone5: ");
    ASSERT_EQ(completion[6], ":clapper: ");
    ASSERT_EQ(completion[7], ":classical_building: ");
}

TEST_F(InputCompletionTest, ClassicTabCompletionCase)
{
    auto completion = queryClassicTabCompletion("pajaw", false);
    ASSERT_EQ(completion.size(), 2);
    ASSERT_EQ(completion[0], "pajaW ");
    ASSERT_EQ(completion[1], "PAJAW ");

    completion = queryClassicTabCompletion("PA", false);
    ASSERT_EQ(completion.size(), 2);
    ASSERT_EQ(completion[0], "pajaW ");
    ASSERT_EQ(completion[1], "PAJAW ");

    completion = queryClassicTabCompletion("Pajaw", false);
    ASSERT_EQ(completion.size(), 2);
    ASSERT_EQ(completion[0], "pajaW ");
    ASSERT_EQ(completion[1], "PAJAW ");

    completion = queryClassicTabCompletion("NOTHING", false);
    ASSERT_EQ(completion.size(), 0);

    completion = queryClassicTabCompletion("nothing", false);
    ASSERT_EQ(completion.size(), 0);
}

TEST_F(InputCompletionTest, SmartEmoteNameFiltering)
{
    auto completion = querySmartEmoteCompletion(":feels");
    ASSERT_EQ(completion.size(), 3);
    ASSERT_EQ(completion[0].displayName, "FeelsBadMan");
    ASSERT_EQ(completion[1].displayName, "FeelsGoodMan");
    ASSERT_EQ(completion[2].displayName, "FeelsBirthdayMan");

    completion = querySmartEmoteCompletion(":)");
    ASSERT_EQ(completion.size(), 3);
    ASSERT_EQ(completion[0].displayName, ":)");
    ASSERT_EQ(completion[1].displayName, ":-)");
    ASSERT_EQ(completion[2].displayName, "B-)");

    completion = querySmartEmoteCompletion(":cat");
    ASSERT_TRUE(completion.size() >= 4);
    ASSERT_EQ(completion[0].displayName, "cat");
    ASSERT_EQ(completion[1].displayName, "cat2");
    ASSERT_EQ(completion[2].displayName, "CatBag");
    ASSERT_EQ(completion[3].displayName, "joy_cat");
}

TEST_F(InputCompletionTest, SmartEmoteExactNameMatching)
{
    auto completion = querySmartEmoteCompletion(":sal");
    ASSERT_TRUE(completion.size() >= 4);
    ASSERT_EQ(completion[0].displayName, "salt");
    ASSERT_EQ(completion[1].displayName, "SaltyCorn");
    ASSERT_EQ(completion[2].displayName, "green_salad");
    ASSERT_EQ(completion[3].displayName, "saluting_face");

    completion = querySmartEmoteCompletion(":salt");
    ASSERT_TRUE(completion.size() >= 2);
    ASSERT_EQ(completion[0].displayName, "salt");
    ASSERT_EQ(completion[1].displayName, "SaltyCorn");
}

TEST_F(InputCompletionTest, SmartEmoteProviderOrdering)
{
    auto completion = querySmartEmoteCompletion(":clap");
    ASSERT_TRUE(completion.size() >= 6);
    ASSERT_EQ(completion[0].displayName, "clap");
    ASSERT_EQ(completion[0].providerName, "Emoji");
    ASSERT_EQ(completion[1].displayName, "Clap");
    ASSERT_EQ(completion[1].providerName, "Global BetterTTV");
    ASSERT_EQ(completion[2].displayName, "Clap");
    ASSERT_EQ(completion[2].providerName, "Global 7TV");
    ASSERT_EQ(completion[3].displayName, "Clap2");
    ASSERT_EQ(completion[3].providerName, "Global 7TV");
    ASSERT_EQ(completion[4].displayName, "clapper");
    ASSERT_EQ(completion[4].providerName, "Emoji");
    ASSERT_EQ(completion[5].displayName, "clap_tone1");
    ASSERT_EQ(completion[5].providerName, "Emoji");
}

TEST_F(InputCompletionTest, SmartEmoteCase)
{
    auto completion = querySmartEmoteCompletion(":pajaw");
    ASSERT_EQ(completion.size(), 2);
    ASSERT_EQ(completion[0].displayName, "pajaW");
    ASSERT_EQ(completion[1].displayName, "PAJAW");

    completion = querySmartEmoteCompletion(":PA");
    ASSERT_EQ(completion.size(), 1);
    ASSERT_EQ(completion[0].displayName, "PAJAW");

    completion = querySmartEmoteCompletion(":Pajaw");
    ASSERT_EQ(completion.size(), 2);
    ASSERT_EQ(completion[0].displayName, "PAJAW");
    ASSERT_EQ(completion[1].displayName, "pajaW");

    completion = querySmartEmoteCompletion(":NOTHING");
    ASSERT_EQ(completion.size(), 0);

    completion = querySmartEmoteCompletion(":nothing");
    ASSERT_EQ(completion.size(), 0);
}

TEST_F(InputCompletionTest, SmartTabCompletionEmote)
{
    auto completion = querySmartTabCompletion(":feels", false);
    ASSERT_EQ(completion.size(), 0);  // : prefix matters here

    // no : prefix defaults to emote completion
    completion = querySmartTabCompletion("feels", false);
    ASSERT_EQ(completion.size(), 3);
    ASSERT_EQ(completion[0], "FeelsBadMan ");
    ASSERT_EQ(completion[1], "FeelsGoodMan ");
    ASSERT_EQ(completion[2], "FeelsBirthdayMan ");

    // no : prefix, emote completion. Duplicate Clap should be removed
    completion = querySmartTabCompletion("cla", false);
    ASSERT_EQ(completion.size(), 3);
    ASSERT_EQ(completion[0], "Clap ");
    ASSERT_EQ(completion[1], "Clap ");
    ASSERT_EQ(completion[2], "Clap2 ");

    completion = querySmartTabCompletion("peepoHappy", false);
    ASSERT_EQ(completion.size(), 0);

    completion = querySmartTabCompletion("Aware", false);
    ASSERT_EQ(completion.size(), 1);
    ASSERT_EQ(completion[0], "Aware ");
}

TEST_F(InputCompletionTest, SmartTabCompletionEmoji)
{
    auto completion = querySmartTabCompletion(":tf", false);
    ASSERT_EQ(completion.size(), 1);
    ASSERT_EQ(completion[0], ":tf: ");

    completion = querySmartTabCompletion(":)", false);
    ASSERT_EQ(completion.size(), 1);
    ASSERT_EQ(completion[0], ":) ");

    completion = querySmartTabCompletion(":cla", false);
    ASSERT_EQ(completion.size(), 8);
    ASSERT_EQ(completion[0], ":clap: ");
    ASSERT_EQ(completion[1], ":clapper: ");
    ASSERT_EQ(completion[2], ":clap_tone1: ");
    ASSERT_EQ(completion[3], ":clap_tone2: ");
    ASSERT_EQ(completion[4], ":clap_tone3: ");
    ASSERT_EQ(completion[5], ":clap_tone4: ");
    ASSERT_EQ(completion[6], ":clap_tone5: ");
    ASSERT_EQ(completion[7], ":classical_building: ");
}

TEST_F(InputCompletionTest, SmartTabCompletionCase)
{
    auto completion = querySmartTabCompletion("pajaw", false);
    ASSERT_EQ(completion.size(), 2);
    ASSERT_EQ(completion[0], "pajaW ");
    ASSERT_EQ(completion[1], "PAJAW ");

    completion = querySmartTabCompletion("PA", false);
    ASSERT_EQ(completion.size(), 1);
    ASSERT_EQ(completion[0], "PAJAW ");

    completion = querySmartTabCompletion("Pajaw", false);
    ASSERT_EQ(completion.size(), 2);
    ASSERT_EQ(completion[0], "PAJAW ");
    ASSERT_EQ(completion[1], "pajaW ");

    completion = querySmartTabCompletion("NOTHING", false);
    ASSERT_EQ(completion.size(), 0);

    completion = querySmartTabCompletion("nothing", false);
    ASSERT_EQ(completion.size(), 0);
}
