#pragma once

#include <QColor>
#include <QLocale>
#include <QString>

#include <chrono>
#include <cmath>
#include <optional>
#include <utility>
#include <vector>

class QDateTime;

namespace chatterino {

// only qualified for tests
namespace helpers::detail {

    using SizeType = QStringView::size_type;

    /**
     * Skips all spaces.
     * The caller must guarantee view.at(startPos).isSpace().
     *
     * @param view The string to skip spaces in.
     * @param startPos The starting position (there must be a space in the view).
     * @return The position of the last space.
     */
    SizeType skipSpace(QStringView view, SizeType startPos);

    /**
     * Checks if `word` equals `expected` (singular) or `expected` + 's' (plural).
     *
     * @param word Word to test. Must not be empty.
     * @param expected Singular of the expected word.
     * @return true if `word` is singular or plural of `expected`.
     */
    bool matchesIgnorePlural(QStringView word, const QString &expected);

    /**
     * Tries to find the unit starting at `pos` and returns its multiplier so
     * `valueInUnit * multiplier = valueInSeconds` (e.g. 60 for minutes).
     *
     * Supported units are
     *      'w[eek(s)]', 'd[ay(s)]',
     *      'h[our(s)]', 'm[inute(s)]', 's[econd(s)]'.
     * The unit must be in lowercase.
     *
     * @param view A view into a string
     * @param pos The starting position.
     *            This is set to the last position of the unit
     *            if it's a valid unit, undefined otherwise.
     * @return (multiplier, ok)
     */
    std::pair<uint64_t, bool> findUnitMultiplierToSec(QStringView view,
                                                      SizeType &pos);

}  // namespace helpers::detail

/**
 * @brief startsWithOrContains is a wrapper for checking
 * whether str1 starts with or contains str2 within itself
 **/
bool startsWithOrContains(QStringView str1, QStringView str2,
                          Qt::CaseSensitivity caseSensitivity, bool startsWith);

/**
 * @brief isNeutral checks if the string doesn't contain any character in the unicode "letter" category
 * i.e. if the string contains only neutral characters.
 **/
bool isNeutral(const QString &s);
QString generateUuid();

QString formatRichLink(const QString &url, bool file = false);

QString formatRichNamedLink(const QString &url, const QString &name,
                            bool file = false);

QString shortenString(const QString &str, unsigned maxWidth = 50);

template <typename T>
QString localizeNumbers(T number)
{
    QLocale locale;
    return locale.toString(number);
}

QString kFormatNumbers(const int &number);

QColor getRandomColor(const QString &userId);

/**
 * Parses a duration.
 * Spaces are allowed before and after a unit but not mandatory.
 * Supported units are
 *      'w[eek(s)]', 'd[ay(s)]',
 *      'h[our(s)]', 'm[inute(s)]', 's[econd(s)]'.
 * Units must be lowercase.
 *
 * If the entire input string is a number (e.g. "12345"),
 * then it's multiplied by noUnitMultiplier.
 *
 * Examples:
 *
 *  - "1w 2h"
 *  - "1w 1w 0s 4d" (2weeks, 4days)
 *  - "5s3h4w" (4weeks, 3hours, 5seconds)
 *  - "30m"
 *  - "1 week"
 *  - "5 days 12 hours"
 *  - "10" (10 * noUnitMultiplier seconds)
 *
 * @param inputString A non-empty string to parse
 * @param noUnitMultiplier A multiplier if the input string only contains one number.
 *                         For example, if a number without a unit should be interpreted
 *                         as a minute, set this to 60. If it should be interpreted
 *                         as a second, set it to 1 (default).
 * @return The parsed duration in seconds, -1 if the input is invalid.
 */
int64_t parseDurationToSeconds(const QString &inputString,
                               uint64_t noUnitMultiplier = 1);

/**
 * @brief Takes a user's name and some formatting parameter and spits out the standardized way to format it
 *
 * @param userName a user's name
 * @param isFirstWord signifies whether this mention would be the first word in a message
 * @param mentionUsersWithComma postfix mentions with a comma. generally powered by getSettings()->mentionUsersWithComma
 **/
QString formatUserMention(const QString &userName, bool isFirstWord,
                          bool mentionUsersWithComma);

template <typename T>
std::vector<T> splitListIntoBatches(const T &list, int batchSize = 100)
{
    std::vector<T> batches;
    int batchCount = std::ceil(static_cast<double>(list.size()) / batchSize);
    batches.reserve(batchCount);

    auto it = list.cbegin();

    for (int j = 0; j < batchCount; j++)
    {
        T batch;

        for (int i = 0; i < batchSize && it != list.end(); i++)
        {
            batch.append(*it);
            it++;
        }
        if (batch.empty())
        {
            break;
        }
        batches.emplace_back(std::move(batch));
    }

    return batches;
}

bool compareEmoteStrings(const QString &a, const QString &b);

template <class T>
constexpr std::optional<T> makeConditionedOptional(bool condition,
                                                   const T &value)
{
    if (condition)
    {
        return value;
    }

    return std::nullopt;
}

template <class T>
constexpr std::optional<std::decay_t<T>> makeConditionedOptional(bool condition,
                                                                 T &&value)
{
    if (condition)
    {
        return std::optional<std::decay_t<T>>(std::forward<T>(value));
    }

    return std::nullopt;
}

/// @brief Unescapes zero width joiners (ZWJ; U+200D) from Twitch messages
///
/// Older Chatterino versions escape ZWJ with an ESCAPE TAG (U+E0002), following
/// https://mm2pl.github.io/emoji_rfc.pdf. This function unescapes all tags with
/// a ZWJ. See also: https://github.com/Chatterino/chatterino2/issues/3384.
QString unescapeZeroWidthJoiner(QString escaped);

QLocale getSystemLocale();

/// @brief Converts `time` to a QDateTime in a local time zone
///
/// Note: When running tests, this will always return a date-time in UTC.
QDateTime chronoToQDateTime(std::chrono::system_clock::time_point time);

/// Slices a string based on codepoint indices.
///
/// If the specified range is outside the string, an empty string view is
/// returned.
///
/// @param begin Start index (inclusive, in codepoints)
/// @param end End index (exclusive, in codepoints)
QStringView codepointSlice(QStringView str, qsizetype begin, qsizetype end);

/// Uses str.removeFirst if Qt >= 6.5, otherwise str.remove(0, 1)
///
/// @param str The Qt string we want to remove 1 character from
void removeFirstQS(QString &str);

/// Uses str.removeLast if Qt >= 6.5, otherwise str.chop(1)
///
/// @param str The Qt string we want to remove 1 character from
void removeLastQS(QString &str);

}  // namespace chatterino
