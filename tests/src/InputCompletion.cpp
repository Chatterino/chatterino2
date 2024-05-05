#include "Application.hpp"
#include "common/Aliases.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/completion/strategies/ClassicEmoteStrategy.hpp"
#include "controllers/completion/strategies/ClassicUserStrategy.hpp"
#include "controllers/completion/strategies/Strategy.hpp"
#include "messages/Emote.hpp"
#include "mocks/Channel.hpp"
#include "mocks/EmptyApplication.hpp"
#include "mocks/Helix.hpp"
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

class MockApplication : mock::EmptyApplication
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

    AccountController accounts;
    mock::MockTwitchIrcServer twitch;
    Emotes emotes;
    BttvEmotes bttvEmotes;
    FfzEmotes ffzEmotes;
    SeventvEmotes seventvEmotes;
};

}  // namespace

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

static QString DEFAULT_SETTINGS = R"!(
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

class InputCompletionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Write default settings to the mock settings json file
        this->settingsDir_ = std::make_unique<QTemporaryDir>();

        QFile settingsFile(this->settingsDir_->filePath("settings.json"));
        ASSERT_TRUE(settingsFile.open(QIODevice::WriteOnly | QIODevice::Text));
        ASSERT_GT(settingsFile.write(DEFAULT_SETTINGS.toUtf8()), 0);
        ASSERT_TRUE(settingsFile.flush());
        settingsFile.close();

        // Initialize helix client
        this->mockHelix = std::make_unique<mock::Helix>();
        initializeHelix(this->mockHelix.get());
        EXPECT_CALL(*this->mockHelix, loadBlocks).Times(Exactly(1));
        EXPECT_CALL(*this->mockHelix, update).Times(Exactly(1));

        this->mockApplication = std::make_unique<MockApplication>();
        this->settings = std::make_unique<Settings>(this->settingsDir_->path());
        this->paths = std::make_unique<Paths>();

        this->mockApplication->accounts.initialize(*this->settings,
                                                   *this->paths);
        this->mockApplication->emotes.initialize(*this->settings, *this->paths);

        this->channelPtr = std::make_shared<MockChannel>("icelys");

        this->initializeEmotes();
    }

    void TearDown() override
    {
        this->mockApplication.reset();
        this->settings.reset();
        this->paths.reset();
        this->mockHelix.reset();
        this->channelPtr.reset();

        this->settingsDir_.reset();
    }

    std::unique_ptr<QTemporaryDir> settingsDir_;

    std::unique_ptr<MockApplication> mockApplication;
    std::unique_ptr<Settings> settings;
    std::unique_ptr<Paths> paths;
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
        this->mockApplication->bttvEmotes.setEmotes(std::move(bttvEmotes));

        auto ffzEmotes = std::make_shared<EmoteMap>();
        addEmote(*ffzEmotes, "LilZ");
        addEmote(*ffzEmotes, "ManChicken");
        addEmote(*ffzEmotes, "CatBag");
        this->mockApplication->ffzEmotes.setEmotes(std::move(ffzEmotes));

        auto seventvEmotes = std::make_shared<EmoteMap>();
        addEmote(*seventvEmotes, "Clap");
        addEmote(*seventvEmotes, "Clap2");
        this->mockApplication->seventvEmotes.setGlobalEmotes(
            std::move(seventvEmotes));
    }

protected:
    auto queryClassicEmoteCompletion(const QString &fullQuery)
    {
        EmoteSource source(this->channelPtr.get(),
                           std::make_unique<ClassicEmoteStrategy>());
        source.update(fullQuery);

        std::vector<EmoteItem> out(source.output());
        return out;
    }

    auto queryClassicTabCompletion(const QString &fullQuery, bool isFirstWord)
    {
        EmoteSource source(this->channelPtr.get(),
                           std::make_unique<ClassicTabEmoteStrategy>());
        source.update(fullQuery);

        QStringList m;
        source.addToStringList(m, 0, isFirstWord);
        return m;
    }
};

void containsRoughly(std::span<EmoteItem> span, std::set<QString> values)
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

TEST_F(InputCompletionTest, ClassicEmoteNameFiltering)
{
    // The completion doesn't guarantee an ordering for a specific category of emotes.
    // This tests a specific implementation of the underlying std::unordered_map,
    // so depending on the standard library used when compiling, this might yield
    // different results.

    auto completion = queryClassicEmoteCompletion(":feels");
    ASSERT_EQ(completion.size(), 3);
    // all these matches are BTTV global emotes
    ASSERT_EQ(completion[0].displayName, "FeelsBirthdayMan");
    ASSERT_EQ(completion[1].displayName, "FeelsBadMan");
    ASSERT_EQ(completion[2].displayName, "FeelsGoodMan");

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
    auto completion = queryClassicTabCompletion(":cla", false);
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
