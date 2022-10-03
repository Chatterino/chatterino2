#pragma once

#include <QColor>
#include <QString>
#include <QStringView>

#include <cmath>

namespace chatterino {

// only qualified for tests
namespace _helpers_internal {

    /**
     * Skips all spaces.
     * The caller must guarantee view.at(startPos).isSpace().
     *
     * @param view The string to skip spaces in.
     * @param startPos The starting position (there must be a space in the view).
     * @return The position of the last space.
     */
    int skipSpace(const QStringView &view, int startPos);

    bool matchesIgnorePlural(const QStringView &word,
                             const QStringView &expected);

    std::pair<uint64_t, bool> findUnitMultiplierToSec(const QStringView &view,
                                                      int &pos);

}  // namespace _helpers_internal

/**
 * @brief startsWithOrContains is a wrapper for checking
 * whether str1 starts with or contains str2 within itself
 **/
bool startsWithOrContains(const QString &str1, const QString &str2,
                          Qt::CaseSensitivity caseSensitivity, bool startsWith);

QString generateUuid();

QString formatRichLink(const QString &url, bool file = false);

QString formatRichNamedLink(const QString &url, const QString &name,
                            bool file = false);

QString shortenString(const QString &str, unsigned maxWidth = 50);

QString localizeNumbers(const int &number);

QString kFormatNumbers(const int &number);

QColor getRandomColor(const QString &userId);

/**
 * Parses a duration.
 * Spaces are disallowed after a number.
 * Spaces are allowed after a unit but not mandatory.
 * Supported units are
 *      'w' (weeks), 'd' (days),
 *      'h' (hours), 'm' (minutes), 's' (seconds).
 *
 * If the entire input string is a number (e.g. "12345"),
 * then it's multiplied by noUnitMultiplier.
 *
 * Examples:
 *
 *  - "1w 2h"
 *  - "1w 1w 0s 4d" (2weeks, 4days)
 *  - "5s3h4w" (4weeks, 3hours, 5seconds)
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

}  // namespace chatterino
