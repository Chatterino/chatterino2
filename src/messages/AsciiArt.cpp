// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "messages/AsciiArt.hpp"

#include <QChar>
#include <QTextBoundaryFinder>

namespace chatterino {

namespace {

constexpr qsizetype MIN_ART_GRAPHEMES = 40;

bool isArtCodePoint(char32_t codePoint)
{
    const auto category = QChar::category(codePoint);
    return category == QChar::Symbol_Other ||
           category == QChar::Symbol_Modifier;
}

qsizetype countArtGraphemes(QStringView text)
{
    QTextBoundaryFinder boundaries(QTextBoundaryFinder::Grapheme, text);
    qsizetype graphemes = 0;
    qsizetype start = 0;

    while (true)
    {
        const auto end = boundaries.toNextBoundary();
        if (end == -1)
        {
            break;
        }

        const auto cluster = text.sliced(start, end - start);
        const auto *pos = cluster.utf16();
        const auto *clusterEnd = pos + cluster.size();
        for (; pos != clusterEnd; ++pos)
        {
            const QChar character = *pos;
            const char32_t codePoint = [&]() -> char32_t {
                if (character.isSurrogate())
                {
                    if (character.isHighSurrogate() && pos + 1 != clusterEnd &&
                        QChar::isLowSurrogate(pos[1]))
                    {
                        const auto highSurrogate = *pos;
                        const auto lowSurrogate = *(++pos);
                        return QChar::surrogateToUcs4(highSurrogate,
                                                      lowSurrogate);
                    }
                    return QChar::ReplacementCharacter;
                }
                return character.unicode();
            }();

            if (isArtCodePoint(codePoint))
            {
                ++graphemes;
                break;
            }
        }
        start = end;
    }

    return graphemes;
}

}  // namespace

bool isAsciiArt(QStringView content)
{
    return countArtGraphemes(content) >= MIN_ART_GRAPHEMES;
}

}  // namespace chatterino
