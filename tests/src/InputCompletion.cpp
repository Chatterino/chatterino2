#include "Application.hpp"
#include "BaseSettings.hpp"
#include "common/Aliases.hpp"
#include "common/CompletionModel.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Emote.hpp"
#include "mocks/EmptyApplication.hpp"
#include "mocks/Helix.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "widgets/splits/InputCompletionPopup.hpp"

#include <boost/optional/optional_io.hpp>
#include <gtest/gtest.h>
#include <QDir>
#include <QFile>
#include <QModelIndex>
#include <QString>
#include <QTemporaryDir>

namespace {

using namespace chatterino;
using ::testing::Exactly;

class MockTwitchIrcServer : public ITwitchIrcServer
{
public:
    const BttvEmotes &getBttvEmotes() const override
    {
        return this->bttv;
    }

    const FfzEmotes &getFfzEmotes() const override
    {
        return this->ffz;
    }

    const SeventvEmotes &getSeventvEmotes() const override
    {
        return this->seventv;
    }

    BttvEmotes bttv;
    FfzEmotes ffz;
    SeventvEmotes seventv;
};

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

    AccountController accounts;
    MockTwitchIrcServer twitch;
    Emotes emotes;
};

}  // namespace

namespace chatterino {

class MockChannel : public Channel
{
public:
    MockChannel(const QString &name)
        : Channel(name, Channel::Type::Twitch)
    {
    }
};

}  // namespace chatterino

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
        this->completionModel =
            std::make_unique<CompletionModel>(*this->channelPtr);

        this->initializeEmotes();
    }

    void TearDown() override
    {
        this->mockApplication.reset();
        this->settings.reset();
        this->paths.reset();
        this->mockHelix.reset();
        this->completionModel.reset();
        this->channelPtr.reset();

        this->settingsDir_.reset();
    }

    std::unique_ptr<QTemporaryDir> settingsDir_;

    std::unique_ptr<MockApplication> mockApplication;
    std::unique_ptr<Settings> settings;
    std::unique_ptr<Paths> paths;
    std::unique_ptr<mock::Helix> mockHelix;

    ChannelPtr channelPtr;
    std::unique_ptr<CompletionModel> completionModel;

private:
    void initializeEmotes()
    {
        auto bttvEmotes = std::make_shared<EmoteMap>();
        addEmote(*bttvEmotes, "FeelsGoodMan");
        addEmote(*bttvEmotes, "FeelsBirthdayMan");
        addEmote(*bttvEmotes, "FeelsBadMan");
        addEmote(*bttvEmotes, "Aware");
        addEmote(*bttvEmotes, "Clueless");
        addEmote(*bttvEmotes, "SaltyCorn");
        addEmote(*bttvEmotes, ":)");
        addEmote(*bttvEmotes, ":-)");
        addEmote(*bttvEmotes, "B-)");
        addEmote(*bttvEmotes, "Clap");
        this->mockApplication->twitch.bttv.setEmotes(std::move(bttvEmotes));

        auto ffzEmotes = std::make_shared<EmoteMap>();
        addEmote(*ffzEmotes, "LilZ");
        addEmote(*ffzEmotes, "ManChicken");
        addEmote(*ffzEmotes, "CatBag");
        this->mockApplication->twitch.ffz.setEmotes(std::move(ffzEmotes));

        auto seventvEmotes = std::make_shared<EmoteMap>();
        addEmote(*seventvEmotes, "Clap");
        addEmote(*seventvEmotes, "Clap2");
        this->mockApplication->twitch.seventv.setGlobalEmotes(
            std::move(seventvEmotes));
    }

protected:
    auto queryEmoteCompletion(const QString &fullQuery)
    {
        // At the moment, buildCompletionEmoteList does not want the ':'.
        QString normalizedQuery = fullQuery;
        if (normalizedQuery.startsWith(':'))
        {
            normalizedQuery = normalizedQuery.mid(1);
        }

        return chatterino::detail::buildCompletionEmoteList(normalizedQuery,
                                                            this->channelPtr);
    }

    auto queryTabCompletion(const QString &fullQuery, bool isFirstWord)
    {
        this->completionModel->refresh(fullQuery, isFirstWord);
        return this->completionModel->allItems();
    }
};

TEST_F(InputCompletionTest, EmoteNameFiltering)
{
    // The completion doesn't guarantee an ordering for a specific category of emotes.
    // This tests a specific implementation of the underlying std::unordered_map,
    // so depending on the standard library used when compiling, this might yield
    // different results.

    auto completion = queryEmoteCompletion(":feels");
    ASSERT_EQ(completion.size(), 3);
    // all these matches are BTTV global emotes
    ASSERT_EQ(completion[0].displayName, "FeelsBirthdayMan");
    ASSERT_EQ(completion[1].displayName, "FeelsBadMan");
    ASSERT_EQ(completion[2].displayName, "FeelsGoodMan");

    completion = queryEmoteCompletion(":)");
    ASSERT_EQ(completion.size(), 3);
    ASSERT_EQ(completion[0].displayName, ":)");  // Exact match with : prefix
    // all these matches are Twitch global emotes
    ASSERT_EQ(completion[1].displayName, ":-)");
    ASSERT_EQ(completion[2].displayName, "B-)");

    completion = queryEmoteCompletion(":cat");
    ASSERT_TRUE(completion.size() >= 2);
    // emoji exact match comes first
    ASSERT_EQ(completion[0].displayName, "cat");
    // FFZ emote is prioritized over any other matching emojis
    ASSERT_EQ(completion[1].displayName, "CatBag");
}

TEST_F(InputCompletionTest, EmoteExactNameMatching)
{
    auto completion = queryEmoteCompletion(":cat");
    ASSERT_TRUE(completion.size() >= 2);
    // emoji exact match comes first
    ASSERT_EQ(completion[0].displayName, "cat");
    // FFZ emote is prioritized over any other matching emojis
    ASSERT_EQ(completion[1].displayName, "CatBag");

    // not exactly "salt", SaltyCorn BTTV emote comes first
    completion = queryEmoteCompletion(":sal");
    ASSERT_TRUE(completion.size() >= 3);
    ASSERT_EQ(completion[0].displayName, "SaltyCorn");
    ASSERT_EQ(completion[1].displayName, "green_salad");
    ASSERT_EQ(completion[2].displayName, "salt");

    // exactly "salt", emoji comes first
    completion = queryEmoteCompletion(":salt");
    ASSERT_TRUE(completion.size() >= 2);
    ASSERT_EQ(completion[0].displayName, "salt");
    ASSERT_EQ(completion[1].displayName, "SaltyCorn");
}

TEST_F(InputCompletionTest, EmoteProviderOrdering)
{
    auto completion = queryEmoteCompletion(":clap");
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

TEST_F(InputCompletionTest, TabCompletionEmote)
{
    auto completion = queryTabCompletion(":feels", false);
    ASSERT_EQ(completion.size(), 0);  // : prefix matters here

    // no : prefix defaults to emote completion
    completion = queryTabCompletion("feels", false);
    ASSERT_EQ(completion.size(), 3);
    // note: different order from : menu
    ASSERT_EQ(completion[0], "FeelsBadMan ");
    ASSERT_EQ(completion[1], "FeelsBirthdayMan ");
    ASSERT_EQ(completion[2], "FeelsGoodMan ");

    // no : prefix, emote completion. Duplicate Clap should be removed
    completion = queryTabCompletion("cla", false);
    ASSERT_EQ(completion.size(), 2);
    ASSERT_EQ(completion[0], "Clap ");
    ASSERT_EQ(completion[1], "Clap2 ");

    completion = queryTabCompletion("peepoHappy", false);
    ASSERT_EQ(completion.size(), 0);  // no peepoHappy emote

    completion = queryTabCompletion("Aware", false);
    ASSERT_EQ(completion.size(), 1);
    ASSERT_EQ(completion[0], "Aware ");  // trailing space added
}

TEST_F(InputCompletionTest, TabCompletionEmoji)
{
    auto completion = queryTabCompletion(":cla", false);
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
