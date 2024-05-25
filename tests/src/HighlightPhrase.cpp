#include "controllers/highlights/HighlightPhrase.hpp"

#include "Test.hpp"

using namespace chatterino;

namespace {

HighlightPhrase buildHighlightPhrase(const QString &phrase, bool isRegex,
                                     bool isCaseSensitive)
{
    return HighlightPhrase(phrase,           // pattern
                           false,            // showInMentions
                           false,            // hasAlert
                           false,            // hasSound
                           isRegex,          // isRegex
                           isCaseSensitive,  // isCaseSensitive
                           "",               // soundURL
                           QColor()          // color
    );
}

}  // namespace

TEST(HighlightPhrase, Normal)
{
    constexpr bool isCaseSensitive = false;
    constexpr bool isRegex = false;

    auto p = buildHighlightPhrase("test", isRegex, isCaseSensitive);

    EXPECT_TRUE(p.isMatch("test"));
    EXPECT_TRUE(p.isMatch("TEst"));
    EXPECT_TRUE(p.isMatch("foo tEst"));
    EXPECT_TRUE(p.isMatch("foo teSt bar"));
    EXPECT_TRUE(p.isMatch("test bar"));

    EXPECT_TRUE(p.isMatch("!teSt"));
    EXPECT_TRUE(p.isMatch("test!"));

    EXPECT_FALSE(p.isMatch("testbar"));
    EXPECT_FALSE(p.isMatch("footest"));
    EXPECT_FALSE(p.isMatch("footestbar"));

    p = buildHighlightPhrase("!test", isRegex, isCaseSensitive);

    EXPECT_TRUE(p.isMatch("!test"));
    EXPECT_TRUE(p.isMatch("foo !test"));
    EXPECT_TRUE(p.isMatch("foo !test bar"));
    EXPECT_TRUE(p.isMatch("!test bar"));

    EXPECT_TRUE(p.isMatch("!test"));
    EXPECT_FALSE(p.isMatch("test!"));

    EXPECT_FALSE(p.isMatch("!testbar"));
    EXPECT_TRUE(p.isMatch(
        "foo!test"));  // Consequence of matching on ! is that before the ! there will be a word boundary, assuming text is smashed right before it EXPECT_FALSE(p.isMatch("foo!testbar"));
    EXPECT_FALSE(p.isMatch("footest!bar"));

    p = buildHighlightPhrase("test!", isRegex, isCaseSensitive);

    EXPECT_TRUE(p.isMatch("test!"));
    EXPECT_TRUE(p.isMatch("foo test!"));
    EXPECT_TRUE(p.isMatch("foo test! bar"));
    EXPECT_TRUE(p.isMatch("test! bar"));

    EXPECT_TRUE(p.isMatch("test!"));
    EXPECT_FALSE(p.isMatch("test"));

    EXPECT_TRUE(p.isMatch(
        "test!bar"));  // Consequence of matching on ! is that before the ! there will be a word boundary, assuming text is smashed right before it
    EXPECT_FALSE(p.isMatch("footest!"));
    EXPECT_FALSE(p.isMatch("footest!bar"));
}

TEST(HighlightPhrase, CaseSensitive)
{
    constexpr bool isCaseSensitive = true;
    constexpr bool isRegex = false;

    using namespace chatterino;

    auto p = buildHighlightPhrase("test", isRegex, isCaseSensitive);

    EXPECT_TRUE(p.isMatch("test"));
    EXPECT_TRUE(p.isMatch("test"));
    EXPECT_TRUE(p.isMatch("foo test"));
    EXPECT_TRUE(p.isMatch("foo test bar"));
    EXPECT_FALSE(p.isMatch("TEst"));
    EXPECT_FALSE(p.isMatch("foo tEst"));
    EXPECT_FALSE(p.isMatch("foo teSt bar"));
    EXPECT_TRUE(p.isMatch("test bar"));

    EXPECT_TRUE(p.isMatch("!test"));
    EXPECT_FALSE(p.isMatch("!teSt"));
    EXPECT_TRUE(p.isMatch("test!"));

    EXPECT_FALSE(p.isMatch("testbar"));
    EXPECT_FALSE(p.isMatch("footest"));
    EXPECT_FALSE(p.isMatch("footestbar"));

    p = buildHighlightPhrase("!test", isRegex, isCaseSensitive);

    EXPECT_FALSE(p.isMatch("!teSt"));
    EXPECT_TRUE(p.isMatch("!test"));
    EXPECT_TRUE(p.isMatch("foo !test"));
    EXPECT_TRUE(p.isMatch("foo !test bar"));
    EXPECT_TRUE(p.isMatch("!test bar"));

    EXPECT_TRUE(p.isMatch("!test"));
    EXPECT_FALSE(p.isMatch("test!"));

    EXPECT_FALSE(p.isMatch("!testbar"));
    EXPECT_TRUE(p.isMatch(
        "foo!test"));  // Consequence of matching on ! is that before the ! there will be a word boundary, assuming text is smashed right before it EXPECT_FALSE(p.isMatch("foo!testbar"));
    EXPECT_FALSE(p.isMatch("footest!bar"));

    p = buildHighlightPhrase("test!", isRegex, isCaseSensitive);

    EXPECT_TRUE(p.isMatch("test!"));
    EXPECT_FALSE(p.isMatch("teSt!"));
    EXPECT_TRUE(p.isMatch("foo test!"));
    EXPECT_TRUE(p.isMatch("foo test! bar"));
    EXPECT_TRUE(p.isMatch("test! bar"));

    EXPECT_TRUE(p.isMatch("test!"));
    EXPECT_FALSE(p.isMatch("test"));

    EXPECT_TRUE(p.isMatch(
        "test!bar"));  // Consequence of matching on ! is that before the ! there will be a word boundary, assuming text is smashed right before it
    EXPECT_FALSE(p.isMatch("footest!"));
    EXPECT_FALSE(p.isMatch("footest!bar"));
}

TEST(HighlightPhrase, Regex)
{
    constexpr bool isCaseSensitive = false;
    constexpr bool isRegex = true;

    using namespace chatterino;

    auto p = buildHighlightPhrase("[a-z]+", isRegex, isCaseSensitive);

    EXPECT_TRUE(p.isMatch("foo"));
    EXPECT_TRUE(p.isMatch("foo bar"));
    EXPECT_FALSE(p.isMatch("!"));

    p = buildHighlightPhrase("^[a-z]+$", isRegex, isCaseSensitive);

    EXPECT_TRUE(p.isMatch("foo"));
    EXPECT_FALSE(p.isMatch("foo bar"));
    EXPECT_FALSE(p.isMatch("!"));

    p = buildHighlightPhrase("^[a-z]+ [a-z]+", isRegex, isCaseSensitive);

    EXPECT_FALSE(p.isMatch("foo"));
    EXPECT_TRUE(p.isMatch("foo bar"));
    EXPECT_TRUE(p.isMatch("foo bar baz"));
    EXPECT_FALSE(p.isMatch("!foo bar"));
    EXPECT_FALSE(p.isMatch("!"));
}
