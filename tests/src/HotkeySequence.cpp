// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/hotkeys/HotkeySequence.hpp"

#include "Test.hpp"

using namespace chatterino;

TEST(HotkeySequence, EmptyByDefault)
{
    HotkeySequence seq;
    EXPECT_TRUE(seq.isEmpty());
    EXPECT_FALSE(seq.isMouse());
    EXPECT_EQ(seq.mouseButton(), Qt::NoButton);
    EXPECT_TRUE(seq.keySequence().isEmpty());
}

TEST(HotkeySequence, KeyboardRoundTrip)
{
    HotkeySequence seq(QKeySequence(QStringLiteral("Ctrl+F")));
    EXPECT_FALSE(seq.isEmpty());
    EXPECT_FALSE(seq.isMouse());
    EXPECT_EQ(seq.toPortableString(), QStringLiteral("Ctrl+F"));

    auto parsed = HotkeySequence::fromPortableString(seq.toPortableString());
    EXPECT_EQ(parsed, seq);
}

TEST(HotkeySequence, MouseBackAliases)
{
    auto fromAlias =
        HotkeySequence::fromPortableString(QStringLiteral("MouseBack"));
    auto fromNumber =
        HotkeySequence::fromPortableString(QStringLiteral("MouseButton4"));
    HotkeySequence fromButton(Qt::BackButton);

    EXPECT_TRUE(fromAlias.isMouse());
    EXPECT_EQ(fromAlias.mouseButton(), Qt::BackButton);
    EXPECT_EQ(fromAlias, fromNumber);
    EXPECT_EQ(fromAlias, fromButton);
    EXPECT_EQ(fromAlias.toPortableString(), QStringLiteral("MouseBack"));
    EXPECT_EQ(fromAlias.toString(), QStringLiteral("Mouse Back"));
}

TEST(HotkeySequence, MouseForwardAliases)
{
    auto fromAlias =
        HotkeySequence::fromPortableString(QStringLiteral("MouseForward"));
    auto fromNumber =
        HotkeySequence::fromPortableString(QStringLiteral("MouseButton5"));
    HotkeySequence fromButton(Qt::ForwardButton);

    EXPECT_EQ(fromAlias, fromNumber);
    EXPECT_EQ(fromAlias, fromButton);
    EXPECT_EQ(fromAlias.toPortableString(), QStringLiteral("MouseForward"));
    EXPECT_EQ(fromAlias.toString(), QStringLiteral("Mouse Forward"));
}

TEST(HotkeySequence, ExtraMouseButtonNumber)
{
    auto seq =
        HotkeySequence::fromPortableString(QStringLiteral("MouseButton6"));
    EXPECT_TRUE(seq.isMouse());
    EXPECT_EQ(seq.mouseButton(), extraMouseButtonFromNumber(6));
    EXPECT_EQ(seq.toPortableString(), QStringLiteral("MouseButton6"));
    EXPECT_EQ(seq.toString(), QStringLiteral("Mouse Button 6"));
}

TEST(HotkeySequence, CaseInsensitiveMouseParse)
{
    auto seq = HotkeySequence::fromPortableString(QStringLiteral("mouseback"));
    EXPECT_EQ(seq.mouseButton(), Qt::BackButton);
}

TEST(HotkeySequence, TrimsPortableString)
{
    auto seq =
        HotkeySequence::fromPortableString(QStringLiteral("  MouseBack  "));
    EXPECT_EQ(seq.mouseButton(), Qt::BackButton);
}

TEST(HotkeySequence, InvalidMouseButtonNumber)
{
    EXPECT_FALSE(extraMouseButtonFromNumber(1).has_value());
    EXPECT_FALSE(extraMouseButtonFromNumber(3).has_value());
    EXPECT_FALSE(extraMouseButtonFromNumber(28).has_value());
    EXPECT_EQ(extraMouseButtonFromNumber(4), Qt::BackButton);
    EXPECT_EQ(extraMouseButtonFromNumber(5), Qt::ForwardButton);
    EXPECT_EQ(extraMouseButtonNumber(Qt::BackButton), 4);
    EXPECT_EQ(extraMouseButtonNumber(Qt::ForwardButton), 5);
    EXPECT_FALSE(extraMouseButtonNumber(Qt::LeftButton).has_value());
    EXPECT_TRUE(extraMouseButtonFromNumber(27).has_value());

    auto parsed =
        HotkeySequence::fromPortableString(QStringLiteral("MouseButton2"));
    EXPECT_TRUE(parsed.isEmpty());
}

TEST(HotkeySequence, RejectsPrimaryMouseButtons)
{
    HotkeySequence left(Qt::LeftButton);
    HotkeySequence right(Qt::RightButton);
    HotkeySequence middle(Qt::MiddleButton);

    EXPECT_TRUE(left.isEmpty());
    EXPECT_TRUE(right.isEmpty());
    EXPECT_TRUE(middle.isEmpty());
    EXPECT_FALSE(isExtraMouseButton(Qt::LeftButton));
    EXPECT_FALSE(isExtraMouseButton(Qt::RightButton));
    EXPECT_FALSE(isExtraMouseButton(Qt::MiddleButton));
    EXPECT_TRUE(isExtraMouseButton(Qt::BackButton));
    EXPECT_TRUE(isExtraMouseButton(Qt::ForwardButton));
}

TEST(HotkeySequence, KeyboardDoesNotCollideWithQtBackKey)
{
    auto mouse =
        HotkeySequence::fromPortableString(QStringLiteral("MouseBack"));
    auto key = HotkeySequence::fromPortableString(QStringLiteral("Back"));
    EXPECT_TRUE(mouse.isMouse());
    EXPECT_FALSE(key.isMouse());
    EXPECT_NE(mouse, key);
}

TEST(HotkeySequence, Equality)
{
    HotkeySequence a(Qt::BackButton);
    HotkeySequence b(Qt::ForwardButton);
    HotkeySequence c(QKeySequence(QStringLiteral("Ctrl+F")));

    EXPECT_EQ(a, HotkeySequence(Qt::BackButton));
    EXPECT_NE(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(b, c);
}
