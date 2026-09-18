// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/ignores/IgnoredEmote.hpp"

#include "mocks/BaseApplication.hpp"
#include "singletons/Settings.hpp"
#include "Test.hpp"

using namespace chatterino;

TEST(IgnoredEmote, LiteralNameMatchesExactly)
{
    const IgnoredEmote ignored{"forsenE", false};
    EXPECT_TRUE(ignored.isMatch("forsenE"));
    EXPECT_FALSE(ignored.isMatch("forsene"));
    EXPECT_FALSE(ignored.isMatch("prefix_forsenE"));
}

TEST(IgnoredEmote, RegexMatchesNames)
{
    const IgnoredEmote ignored{"^forsen.*", true};
    EXPECT_TRUE(ignored.isMatch("forsenE"));
    EXPECT_FALSE(ignored.isMatch("x_forsenE"));
}

TEST(IgnoredEmote, EmptyOrInvalidPatternDoesNotMatch)
{
    const IgnoredEmote emptyLiteral{"", false};
    const IgnoredEmote emptyRegex{"", true};
    const IgnoredEmote invalidRegex{"[", true};
    EXPECT_FALSE(emptyLiteral.isMatch("forsenE"));
    EXPECT_FALSE(emptyRegex.isMatch("forsenE"));
    EXPECT_FALSE(invalidRegex.isMatch("forsenE"));
}

TEST(IgnoredEmote, SettingsMatchesNames)
{
    mock::BaseApplication app;
    app.settings.ignoredEmotes.append(IgnoredEmote{"forsenE", false});
    EXPECT_TRUE(app.settings.isEmoteIgnored("forsenE"));
    EXPECT_FALSE(app.settings.isEmoteIgnored("forsene"));
    EXPECT_FALSE(app.settings.isEmoteIgnored("prefix_forsenE"));

    app.settings.ignoredEmotes.append(IgnoredEmote{"^pepe.*", true});
    EXPECT_TRUE(app.settings.isEmoteIgnored("pepeLaugh"));
    EXPECT_FALSE(app.settings.isEmoteIgnored("x_pepeLaugh"));
}

TEST(IgnoredEmote, UncheckingRemovesDuplicateNames)
{
    mock::BaseApplication app;
    auto &settings = app.settings;
    settings.ignoredEmotes.append(IgnoredEmote{"forsenE", false});
    settings.ignoredEmotes.append(IgnoredEmote{"forsenE", false});
    settings.ignoredEmotes.append(IgnoredEmote{"^forsenE$", true});

    settings.setEmoteNameIgnored("forsenE", false);
    ASSERT_EQ(settings.ignoredEmotes.raw().size(), 1);
    EXPECT_TRUE(settings.ignoredEmotes.raw().front().regex);

    settings.setEmoteNameIgnored("forsenE", true);
    settings.setEmoteNameIgnored("forsenE", true);
    EXPECT_EQ(settings.ignoredEmotes.raw().size(), 2);
}
