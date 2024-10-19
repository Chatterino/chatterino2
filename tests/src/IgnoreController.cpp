#include "controllers/ignores/IgnoreController.hpp"

#include "controllers/accounts/AccountController.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/Emotes.hpp"
#include "providers/twitch/TwitchIrc.hpp"
#include "Test.hpp"

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

    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

    mock::Emotes emotes;
    AccountController accounts;
};

}  // namespace

class TestIgnoreController : public ::testing::Test
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

TEST_F(TestIgnoreController, processIgnorePhrases)
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
        processIgnorePhrases(test.phrases, message, emotes);

        EXPECT_EQ(message, test.expectedMessage)
            << "Message not equal for input '" << test.input
            << "' - expected: '" << test.expectedMessage << "' got: '"
            << message << "'";
        EXPECT_EQ(emotes, test.expectedTwitchEmotes)
            << "Twitch emotes not equal for input '" << test.input
            << "' and output '" << message << "'";
    }
}
