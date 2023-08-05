#include "providers/autocomplete/AutocompleteStrategies.hpp"

#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"

namespace chatterino {

//// Emote strategies

void ClassicAutocompleteEmoteStrategy::apply(
    const std::vector<CompletionEmote> &items,
    std::vector<CompletionEmote> &output, const QString &query) const
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
    bool operator()(const CompletionEmote &a, const CompletionEmote &b) const
    {
        return compareEmoteStrings(a.searchName, b.searchName);
    }
};

void ClassicTabAutocompleteEmoteStrategy::apply(
    const std::vector<CompletionEmote> &items,
    std::vector<CompletionEmote> &output, const QString &query) const
{
    bool emojiOnly = false;
    QString normalizedQuery = query;
    if (normalizedQuery.startsWith(':'))
    {
        normalizedQuery = normalizedQuery.mid(1);
        // tab completion with : prefix should do emojis only
        emojiOnly = true;
    }

    std::set<CompletionEmote, CompletionEmoteOrder> emotes;

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

//// User strategies

void ClassicAutocompleteUserStrategy::apply(
    const std::vector<UsersAutocompleteItem> &items,
    std::vector<UsersAutocompleteItem> &output, const QString &query) const
{
    QString lowerQuery = query.toLower();
    if (lowerQuery.startsWith('@'))
    {
        lowerQuery = lowerQuery.mid(1);
    }

    for (const auto &item : items)
    {
        if (item.first.startsWith(lowerQuery))
        {
            output.push_back(item);
        }
    }
}

//// Command strategies

AutocompleteCommandStrategy::AutocompleteCommandStrategy(bool startsWithOnly)
    : startsWithOnly_(startsWithOnly)
{
}

void AutocompleteCommandStrategy::apply(
    const std::vector<CompletionCommand> &items,
    std::vector<CompletionCommand> &output, const QString &query) const
{
    QString normalizedQuery = query;
    if (normalizedQuery.startsWith('/') || normalizedQuery.startsWith('.'))
    {
        normalizedQuery = normalizedQuery.mid(1);
    }

    if (startsWithOnly_)
    {
        std::copy_if(items.begin(), items.end(),
                     std::back_insert_iterator(output),
                     [&normalizedQuery](const CompletionCommand &item) {
                         return item.name.startsWith(normalizedQuery,
                                                     Qt::CaseInsensitive);
                     });
    }
    else
    {
        std::copy_if(
            items.begin(), items.end(), std::back_insert_iterator(output),
            [&normalizedQuery](const CompletionCommand &item) {
                return item.name.contains(normalizedQuery, Qt::CaseInsensitive);
            });
    }
};

}  // namespace chatterino