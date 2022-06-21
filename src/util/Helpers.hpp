#pragma once

#include <QColor>
#include <QString>

#include <cmath>

namespace chatterino {

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
 * @brief Takes a user's name and some formatting parameter and spits out the standardized way to format it
 *
 * @param userName a user's name
 * @param isFirstWord signifies whether this mention would be the first word in a message
 * @param mentionUsersWithComma postfix mentions with a comma. generally powered by getSettings()->mentionUsersWithComma
 * @param lowercaseUsernames [dankerino] lowercase the resulting string
 **/
QString formatUserMention(const QString &userName, bool isFirstWord,
                          bool mentionUsersWithComma, bool lowercaseUsernames);

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
