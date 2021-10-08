#include "util/Helpers.hpp"

#include <gtest/gtest.h>

using namespace chatterino;

TEST(Helpers, formatUserMention)
{
    const auto userName = "Pajlada";

    // A user mention that is the first word, that has 'mention with comma' enabled should have a comma appended at the end.
    EXPECT_EQ(formatUserMention(userName, true, true, false), "Pajlada,");
    EXPECT_EQ(formatUserMention(userName, true, true, true), "pajlada,");

    // A user mention that is not the first word, but has 'mention with comma' enabled should not have a comma appended at the end.
    EXPECT_EQ(formatUserMention(userName, false, true, false), "Pajlada");
    EXPECT_EQ(formatUserMention(userName, false, true, true), "pajlada");

    // A user mention that is the first word, but has 'mention with comma' disabled should not have a comma appended at the end.
    EXPECT_EQ(formatUserMention(userName, true, false, false), "Pajlada");
    EXPECT_EQ(formatUserMention(userName, true, false, true), "pajlada");

    // A user mention that is neither the first word, nor has 'mention with comma' enabled should not have a comma appended at the end.
    EXPECT_EQ(formatUserMention(userName, false, false, false), "Pajlada");
    EXPECT_EQ(formatUserMention(userName, false, false, true), "pajlada");
}
