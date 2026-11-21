// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "messages/AsciiArt.hpp"

#include "common/Literals.hpp"
#include "Test.hpp"

using namespace chatterino::literals;
using chatterino::isAsciiArt;

TEST(AsciiArt, RejectsAmbiguousMessages)
{
    EXPECT_FALSE(isAsciiArt(u"ordinary prose ⣿⣿⣿"_s));
    const auto modifiedHand = u"👉🏿"_s;
    EXPECT_FALSE(isAsciiArt(modifiedHand.repeated(10) + u' ' +
                            modifiedHand.repeated(10)));
}

TEST(AsciiArt, DetectsMixedUnicodeArt)
{
    const QString brailleSegment(20, QChar(0x28FF));
    const QString blockSegment(20, QChar(0x25AC));
    const auto longText = QString(200, u'x');
    EXPECT_TRUE(isAsciiArt(brailleSegment + brailleSegment));
    EXPECT_TRUE(
        isAsciiArt(blockSegment + u' ' + longText + u' ' + blockSegment));

    const auto rowWithBlank = brailleSegment + QChar(0x2800) + brailleSegment;
    EXPECT_TRUE(isAsciiArt(brailleSegment + u" mixed text "_s + rowWithBlank));
}

TEST(AsciiArt, DetectsEmojiArtByGrapheme)
{
    const auto modifiedHand = u"👉🏿"_s;
    EXPECT_TRUE(isAsciiArt(modifiedHand.repeated(20) + u' ' +
                           modifiedHand.repeated(20)));
}
