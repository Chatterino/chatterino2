#include "controllers/completion/strategies/ClassicEmoteStrategy.hpp"

#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"

namespace chatterino::completion {

void ClassicEmoteStrategy::apply(const std::vector<EmoteItem> &items,
                                 std::vector<EmoteItem> &output,
                                 const QString &query) const
{
    QString normalizedQuery = query;
    if (normalizedQuery.startsWith(':'))
    {
        normalizedQuery = normalizedQuery.mid(1);
    }

    // First pass: filter by contains match
    for (const auto &item : items)
    {
        if (item.searchName.contains(normalizedQuery, Qt::CaseInsensitive))
        {
            output.push_back(item);
        }
    }

    // Second pass: if there is an exact match, put that emote first
    for (size_t i = 1; i < output.size(); i++)
    {
        auto emoteText = output.at(i).searchName;

        // test for match or match with colon at start for emotes like ":)"
        if (emoteText.compare(normalizedQuery, Qt::CaseInsensitive) == 0 ||
            emoteText.compare(":" + normalizedQuery, Qt::CaseInsensitive) == 0)
        {
            auto emote = output[i];
            output.erase(output.begin() + int(i));
            output.insert(output.begin(), emote);
            break;
        }
    }
}

struct CompletionEmoteOrder {
    bool operator()(const EmoteItem &a, const EmoteItem &b) const
    {
        return compareEmoteStrings(a.searchName, b.searchName);
    }
};

void ClassicTabEmoteStrategy::apply(const std::vector<EmoteItem> &items,
                                    std::vector<EmoteItem> &output,
                                    const QString &query) const
{
    bool emojiOnly = false;
    QString normalizedQuery = query;
    if (normalizedQuery.startsWith(':'))
    {
        normalizedQuery = normalizedQuery.mid(1);
        // tab completion with : prefix should do emojis only
        emojiOnly = true;
    }

    std::set<EmoteItem, CompletionEmoteOrder> emotes;

    for (const auto &item : items)
    {
        if (emojiOnly ^ item.isEmoji)
        {
            continue;
        }

        if (startsWithOrContains(item.searchName, normalizedQuery,
                                 Qt::CaseInsensitive,
                                 getSettings()->prefixOnlyEmoteCompletion))
        {
            emotes.insert(item);
        }
    }

    output.reserve(emotes.size());
    output.assign(emotes.begin(), emotes.end());
}

}  // namespace chatterino::completion
