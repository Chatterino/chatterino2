#include "controllers/filters/lang/Filter.hpp"
#include "controllers/filters/lang/Types.hpp"

#include <gtest/gtest.h>
#include <QColor>
#include <QVariant>

using namespace chatterino;
using namespace chatterino::filters;

TypingContext typingContext = MESSAGE_TYPING_CONTEXT;

namespace chatterino::filters {

std::ostream &operator<<(std::ostream &os, Type t)
{
    os << qUtf8Printable(typeToString(t));
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
            << "Filter::fromString( " << qUtf8Printable(input)
            << " ) should be " << (expected ? "valid" : "invalid");
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
        ASSERT_TRUE(isValid) << "Filter::fromString( " << qUtf8Printable(input)
                             << " ) is invalid";

        auto filter = std::move(std::get<Filter>(filterResult));
        T type = filter.returnType();
        EXPECT_EQ(type, expected)
            << "Filter{ " << qUtf8Printable(input) << " } has type " << type
            << " instead of " << expected << ".\nDebug: "
            << qUtf8Printable(filter.debugString(typingContext));
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
        ASSERT_TRUE(isValid) << "Filter::fromString( " << qUtf8Printable(input)
                             << " ) is invalid";

        auto filter = std::move(std::get<Filter>(filterResult));
        auto result = filter.execute(contextMap);

        EXPECT_EQ(result, expected)
            << "Filter{ " << qUtf8Printable(input) << " } evaluated to "
            << qUtf8Printable(result.toString()) << " instead of "
            << qUtf8Printable(expected.toString()) << ".\nDebug: "
            << qUtf8Printable(filter.debugString(typingContext));
    }
}
