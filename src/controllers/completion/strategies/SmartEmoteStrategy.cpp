#include "controllers/completion/strategies/SmartEmoteStrategy.hpp"

#include "common/QLogging.hpp"
#include "controllers/completion/sources/EmoteSource.hpp"
#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"

#include <Qt>

#include <algorithm>

namespace chatterino::completion {
namespace {
    /**
     * @brief This function calculates the "cost" of the changes that need to
     * be done to the query to make it the value.
     *
     * By default an emote with more differences in character casing from the
     * query will get a higher cost, each additional letter also increases cost.
     *
     * @param prioritizeUpper If set, then differences in casing don't matter, but
     * instead the more lowercase letters an emote contains, the higher cost it
     * will get. Additional letters also increase the cost in this mode.
     *
     * @return How different the emote is from query. Values in the range [-10,
     * \infty].
     */
    int costOfEmote(QStringView query, QStringView emote, bool prioritizeUpper)
    {
        int score = 0;

        if (prioritizeUpper)
        {
            // We are in case 3, push 'more uppercase' emotes to the top
            for (const auto i : emote)
            {
                score += int(!i.isUpper());
            }
        }
        else
        {
            // Push more matching emotes to the top
            int len = std::min(emote.size(), query.size());
            for (int i = 0; i < len; i++)
            {
                // Different casing gets a higher cost score
                score += query.at(i).isUpper() ^ emote.at(i).isUpper();
            }
        }
        // No case differences, put this at the top
        if (score == 0)
        {
            score = -10;
        }

        auto diff = emote.size() - query.size();
        if (diff > 0)
        {
            // Case changes are way less changes to the user compared to adding characters
            score += diff * 100;
        }
        return score;
    };

    // This contains the brains of emote tab completion. Updates output to sorted completions.
    // Ensure that the query string is already normalized, that is doesn't have a leading ':'
    // matchingFunction is used for testing if the emote should be included in the search.
    void completeEmotes(
        const std::vector<EmoteItem> &items, std::vector<EmoteItem> &output,
        QStringView query, bool ignoreColonForCost,
        const std::function<bool(EmoteItem, Qt::CaseSensitivity)>
            &matchingFunction)
    {
        // Given these emotes: pajaW, PAJAW
        // There are a few cases of input:
        // 1. "pajaw" expect {pajaW, PAJAW} - no uppercase characters, do regular case insensitive search
        // 2. "PA" expect {PAJAW}           - uppercase characters, case sensitive search gives results
        // 3. "Pajaw" expect {PAJAW, pajaW} - case sensitive search doesn't give results, need to use sorting
        // 4. "NOTHING" expect {}           - no results
        // 5. "nothing" expect {}           - same as 4 but first search is case insensitive

        // Check if the query contains any uppercase characters
        // This tells us if we're in case 1 or 5 vs all others
        bool haveUpper =
            std::any_of(query.begin(), query.end(), [](const QChar &c) {
                return c.isUpper();
            });

        // First search, for case 1 it will be case insensitive,
        // for cases 2, 3 and 4 it will be case sensitive
        for (const auto &item : items)
        {
            if (matchingFunction(
                    item, haveUpper ? Qt::CaseSensitive : Qt::CaseInsensitive))
            {
                output.push_back(item);
            }
        }

        // if case 3: then true; false otherwise
        bool prioritizeUpper = false;

        // No results from search
        if (output.empty())
        {
            if (!haveUpper)
            {
                // Optimisation: First search was case insensitive, but we found nothing
                // There is nothing to be found: case 5.
                return;
            }
            // Case sensitive search from case 2 found nothing, therefore we can
            // only be in case 3 or 4.

            prioritizeUpper = true;
            // Run the search again but this time without case sensitivity
            for (const auto &item : items)
            {
                if (matchingFunction(item, Qt::CaseInsensitive))
                {
                    output.push_back(item);
                }
            }
            if (output.empty())
            {
                // The second search found nothing, so don't even try to sort: case 4
                return;
            }
        }

        std::sort(output.begin(), output.end(),
                  [query, prioritizeUpper, ignoreColonForCost](
                      const EmoteItem &a, const EmoteItem &b) -> bool {
                      auto tempA = a.searchName;
                      auto tempB = b.searchName;
                      if (ignoreColonForCost && tempA.startsWith(":"))
                      {
                          tempA = tempA.mid(1);
                      }
                      if (ignoreColonForCost && tempB.startsWith(":"))
                      {
                          tempB = tempB.mid(1);
                      }

                      auto costA = costOfEmote(query, tempA, prioritizeUpper);
                      auto costB = costOfEmote(query, tempB, prioritizeUpper);
                      if (costA == costB)
                      {
                          // Case difference and length came up tied for (a, b), break the tie
                          return QString::compare(tempA, tempB,
                                                  Qt::CaseInsensitive) < 0;
                      }

                      return costA < costB;
                  });
    }
}  // namespace

void SmartEmoteStrategy::apply(const std::vector<EmoteItem> &items,
                               std::vector<EmoteItem> &output,
                               const QString &query) const
{
    QString normalizedQuery = query;
    bool ignoreColonForCost = false;
    if (normalizedQuery.startsWith(':'))
    {
        normalizedQuery = normalizedQuery.mid(1);
        ignoreColonForCost = true;
    }
    completeEmotes(items, output, normalizedQuery, ignoreColonForCost,
                   [normalizedQuery](const EmoteItem &left,
                                     Qt::CaseSensitivity caseHandling) {
                       return left.searchName.contains(normalizedQuery,
                                                       caseHandling);
                   });
}

void SmartTabEmoteStrategy::apply(const std::vector<EmoteItem> &items,
                                  std::vector<EmoteItem> &output,
                                  const QString &query) const
{
    bool colonStart = query.startsWith(':');
    QStringView normalizedQuery = query;
    if (colonStart)
    {
        // TODO(Qt6): use sliced
        normalizedQuery = normalizedQuery.mid(1);
    }

    completeEmotes(
        items, output, normalizedQuery, false,
        [&](const EmoteItem &item, Qt::CaseSensitivity caseHandling) -> bool {
            QStringView itemQuery;
            if (item.isEmoji)
            {
                if (colonStart)
                {
                    itemQuery = normalizedQuery;
                }
                else
                {
                    return false;  // ignore emojis when not completing with ':'
                }
            }
            else
            {
                itemQuery = query;
            }

            return startsWithOrContains(
                item.searchName, itemQuery, caseHandling,
                getSettings()->prefixOnlyEmoteCompletion);
        });
}

}  // namespace chatterino::completion
