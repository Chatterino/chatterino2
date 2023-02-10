#include "controllers/filters/lang/Filter.hpp"
#include "controllers/filters/lang/Types.hpp"

#include <gtest/gtest.h>
#include <QColor>
#include <QVariant>

using namespace chatterino;
using namespace chatterino::filters;

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
        {R".(author.badges).", T::List},
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
            << " instead of " << expected;
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
        {R".(1 + 1).", QVariant(2)}, 
        {R".(2 + 3 * 4).", QVariant(20)},  // math operators have the same precedence
        {R".(1 > 2 || 3 > 1).", QVariant(true)}, 
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
            << qUtf8Printable(expected.toString());
    }
}
