#include "controllers/accounts/AccountController.hpp"
#include "controllers/filters/lang/expressions/UnaryOperation.hpp"
#include "controllers/filters/lang/Filter.hpp"
#include "controllers/filters/lang/Types.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "messages/MessageBuilder.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/Channel.hpp"
#include "mocks/ChatterinoBadges.hpp"
#include "mocks/Emotes.hpp"
#include "mocks/EmptyApplication.hpp"
#include "mocks/Logging.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "mocks/UserData.hpp"
#include "providers/ffz/FfzBadges.hpp"
#include "providers/seventv/SeventvBadges.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "Test.hpp"

#include <QColor>
#include <QVariant>

using namespace chatterino;
using namespace chatterino::filters;
using chatterino::mock::MockChannel;

namespace {

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

    ILogging *getChatLogger() override
    {
        return &this->logging;
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
};

class FiltersF : public ::testing::Test
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

}  // namespace

namespace chatterino::filters {

std::ostream &operator<<(std::ostream &os, Type t)
{
    os << typeToString(t);
    return os;
}

}  // namespace chatterino::filters

TEST(Filters, Validity)
{
    struct TestCase {
        QString input;
        bool valid;
    };

    // clang-format off
    std::vector<TestCase> tests{
        {"", false},
        {R".(1 + 1).", true},
        {R".(1 + ).", false},
        {R".(1 + 1)).", false},
        {R".((1 + 1).", false},
        {R".(author.name contains "icelys").", true},
        {R".(author.color == "#ff0000").", true},
        {R".(author.name - 5).", false},  // can't perform String - Int
        {R".(message.content match {r"(\d\d)/(\d\d)/(\d\d\d\d)", 3}).", true},
        {R".("abc" + 123 == "abc123").", true},
        {R".(123 + "abc" == "hello").", false},
        {R".(flags.reply && flags.automod).", true},
        {R".(unknown.identifier).", false},
        {R".(channel.name == "forsen" && author.badges contains "moderator").", true},
    };
    // clang-format on

    for (const auto &[input, expected] : tests)
    {
        auto filterResult = Filter::fromString(input);
        bool isValid = std::holds_alternative<Filter>(filterResult);
        EXPECT_EQ(isValid, expected)
            << "Filter::fromString( " << input << " ) should be "
            << (expected ? "valid" : "invalid");
    }
}

TEST(Filters, TypeSynthesis)
{
    using T = Type;
    struct TestCase {
        QString input;
        T type;
    };

    // clang-format off
    std::vector<TestCase> tests
    {
        {R".(1 + 1).", T::Int},
        {R".(author.color).", T::Color},
        {R".(author.name).", T::String},
        {R".(!author.subbed).", T::Bool},
        {R".(author.badges).", T::StringList},
        {R".(channel.name == "forsen" && author.badges contains "moderator").", T::Bool},
        {R".(message.content match {r"(\d\d)/(\d\d)/(\d\d\d\d)", 3}).", T::String},
    };
    // clang-format on

    for (const auto &[input, expected] : tests)
    {
        auto filterResult = Filter::fromString(input);
        bool isValid = std::holds_alternative<Filter>(filterResult);
        ASSERT_TRUE(isValid)
            << "Filter::fromString( " << input << " ) is invalid";

        auto filter = std::move(std::get<Filter>(filterResult));
        T type = filter.returnType();
        EXPECT_EQ(type, expected)
            << "Filter{ " << input << " } has type " << type << " instead of "
            << expected
            << ".\nDebug: " << filter.debugString(MESSAGE_TYPING_CONTEXT);
    }
}

TEST(Filters, Evaluation)
{
    struct TestCase {
        QString input;
        QVariant output;
    };

    ContextMap contextMap = {
        {"author.name", QVariant("icelys")},
        {"author.color", QVariant(QColor("#ff0000"))},
        {"author.subbed", QVariant(false)},
        {"message.content", QVariant("hey there :) 2038-01-19 123 456")},
        {"channel.name", QVariant("forsen")},
        {"author.badges", QVariant(QStringList({"moderator", "staff"}))}};

    // clang-format off
    std::vector<TestCase> tests
    {
        // Evaluation semantics
        {R".(1 + 1).", QVariant(2)},
        {R".(!(1 == 1)).", QVariant(false)},
        {R".(2 + 3 * 4).", QVariant(20)},  // math operators have the same precedence
        {R".(1 > 2 || 3 >= 3).", QVariant(true)},
        {R".(1 > 2 && 3 > 1).", QVariant(false)},
        {R".("abc" + 123).", QVariant("abc123")},
        {R".("abc" + "456").", QVariant("abc456")},
        {R".(3 - 4).", QVariant(-1)},
        {R".(3 * 4).", QVariant(12)},
        {R".(8 / 3).", QVariant(2)},
        {R".(7 % 3).", QVariant(1)},
        {R".(5 == 5).", QVariant(true)},
        {R".(5 == "5").", QVariant(true)},
        {R".(5 != 7).", QVariant(true)},
        {R".(5 == "abc").", QVariant(false)},
        {R".("ABC123" == "abc123").", QVariant(true)},  // String comparison is case-insensitive
        {R".("Hello world" contains "Hello").", QVariant(true)},
        {R".("Hello world" contains "LLO W").", QVariant(true)},  // Case-insensitive
        {R".({"abc", "def"} contains "abc").", QVariant(true)},
        {R".({"abc", "def"} contains "ABC").", QVariant(true)},  // Case-insensitive when list is all strings
        {R".({123, "def"} contains "DEF").", QVariant(false)},  // Case-sensitive if list not all strings
        {R".({"a123", "b456"} startswith "a123").", QVariant(true)},
        {R".({"a123", "b456"} startswith "A123").", QVariant(true)},
        {R".({} startswith "A123").", QVariant(false)},
        {R".("Hello world" startswith "Hello").", QVariant(true)},
        {R".("Hello world" startswith "world").", QVariant(false)},
        {R".({"a123", "b456"} endswith "b456").", QVariant(true)},
        {R".({"a123", "b456"} endswith "B456").", QVariant(true)},
        {R".("Hello world" endswith "world").", QVariant(true)},
        {R".("Hello world" endswith "Hello").", QVariant(false)},
        // Context map usage
        {R".(author.name).", QVariant("icelys")},
        {R".(!author.subbed).", QVariant(true)},
        {R".(author.color == "#ff0000").", QVariant(true)},
        {R".(channel.name == "forsen" && author.badges contains "moderator").", QVariant(true)},
        {R".(message.content match {r"(\d\d\d\d)\-(\d\d)\-(\d\d)", 3}).", QVariant("19")},
        {R".(message.content match r"HEY THERE").", QVariant(false)},
        {R".(message.content match ri"HEY THERE").", QVariant(true)},
    };
    // clang-format on

    for (const auto &[input, expected] : tests)
    {
        auto filterResult = Filter::fromString(input);
        bool isValid = std::holds_alternative<Filter>(filterResult);
        ASSERT_TRUE(isValid)
            << "Filter::fromString( " << input << " ) is invalid";

        auto filter = std::move(std::get<Filter>(filterResult));
        auto result = filter.execute(contextMap);

        EXPECT_EQ(result, expected)
            << "Filter{ " << input << " } evaluated to " << result.toString()
            << " instead of " << expected.toString()
            << ".\nDebug: " << filter.debugString(MESSAGE_TYPING_CONTEXT);
    }
}

TEST_F(FiltersF, TypingContextChecks)
{
    MockChannel channel("pajlada");

    QByteArray message =
        R"(@badge-info=subscriber/80;badges=broadcaster/1,subscriber/3072,partner/1;color=#CC44FF;display-name=pajlada;emote-only=1;emotes=25:0-4;first-msg=0;flags=;id=90ef1e46-8baa-4bf2-9c54-272f39d6fa11;mod=0;returning-chatter=0;room-id=11148817;subscriber=1;tmi-sent-ts=1662206235860;turbo=0;user-id=11148817;user-type= :pajlada!pajlada@pajlada.tmi.twitch.tv PRIVMSG #pajlada :ACTION Kappa)";

    struct TestCase {
        QByteArray input;
    };

    auto *privmsg = dynamic_cast<Communi::IrcPrivateMessage *>(
        Communi::IrcPrivateMessage::fromData(message, nullptr));
    EXPECT_NE(privmsg, nullptr);

    QString originalMessage = privmsg->content();

    auto [msg, alert] = MessageBuilder::makeIrcMessage(
        &channel, privmsg, MessageParseArgs{}, originalMessage, 0);

    EXPECT_NE(msg.get(), nullptr);

    auto contextMap = buildContextMap(msg, &channel);

    EXPECT_EQ(contextMap.size(), MESSAGE_TYPING_CONTEXT.size());

    delete privmsg;
}

TEST_F(FiltersF, ExpressionDebug)
{
    struct TestCase {
        QString input;
        QString debugString;
        QString filterString;
    };

    // clang-format off
    std::vector<TestCase> tests{
        {
            .input = R".(1 + 1).",
            .debugString = "BinaryOp[Plus](Val(1) : Int, Val(1) : Int)",
            .filterString = "(1 + 1)",
        },
        {
            .input = R".(author.color == "#ff0000").",
            .debugString = "BinaryOp[Eq](Val(author.color) : Color, Val(#ff0000) : String)",
            .filterString = R".((author.color == "#ff0000")).",
        },
        {
            .input = R".(1).",
            .debugString = "Val(1)",
            .filterString = R".(1).",
        },
        {
            .input = R".("asd").",
            .debugString = R".(Val(asd)).",
            .filterString = R".("asd").",
        },
        {
            .input = R".(("asd")).",
            .debugString = R".(Val(asd)).",
            .filterString = R".("asd").",
        },
        {
            .input = R".(author.subbed).",
            .debugString = R".(Val(author.subbed)).",
            .filterString = R".(author.subbed).",
        },
        {
            .input = R".(!author.subbed).",
            .debugString = R".(UnaryOp[Not](Val(author.subbed) : Bool)).",
            .filterString = R".((!author.subbed)).",
        },
        {
            .input = R".({"foo", "bar"} contains "foo").",
            .debugString = R".(BinaryOp[Contains](List(Val(foo) : String, Val(bar) : String) : StringList, Val(foo) : String)).",
            .filterString = R".(({"foo", "bar"} contains "foo")).",
        },
        {
            .input = R".(!({"foo", "bar"} contains "foo")).",
            .debugString = R".(UnaryOp[Not](BinaryOp[Contains](List(Val(foo) : String, Val(bar) : String) : StringList, Val(foo) : String) : Bool)).",
            .filterString = R".((!({"foo", "bar"} contains "foo"))).",
        },
        {
            .input = R".(message.content match r"(\d\d)/(\d\d)/(\d\d\d\d)").",
            .debugString = R".(BinaryOp[Match](Val(message.content) : String, RegEx((\d\d)/(\d\d)/(\d\d\d\d)) : RegularExpression)).",
            .filterString = R".((message.content match r"(\d\d)/(\d\d)/(\d\d\d\d)")).",
        },
    };
    // clang-format on

    for (const auto &[input, debugString, filterString] : tests)
    {
        const auto filterResult = Filter::fromString(input);
        const auto *filter = std::get_if<Filter>(&filterResult);
        EXPECT_NE(filter, nullptr) << "Filter::fromString(" << input
                                   << ") did not build a proper filter";

        const auto actualDebugString =
            filter->debugString(MESSAGE_TYPING_CONTEXT);
        EXPECT_EQ(actualDebugString, debugString)
            << "filter->debugString() on '" << input << "' should be '"
            << debugString << "', but got '" << actualDebugString << "'";

        const auto actualFilterString = filter->filterString();
        EXPECT_EQ(actualFilterString, filterString)
            << "filter->filterString() on '" << input << "' should be '"
            << filterString << "', but got '" << actualFilterString << "'";
    }
}
