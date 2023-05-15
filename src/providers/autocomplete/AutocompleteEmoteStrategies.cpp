#include "providers/autocomplete/AutocompleteEmoteStrategies.hpp"

namespace chatterino {

void ClassicAutocompleteEmoteStrategy::apply(
    const std::vector<CompletionEmote> &items,
    std::vector<CompletionEmote> &output, const QString &query) const
{
    // First pass: filter by contains match
    for (const auto &item : items)
    {
        if (item.name.contains(query, Qt::CaseInsensitive))
        {
            output.push_back(item);
        }
    }

    // Second pass: if there is an exact match, put that emote first
    for (size_t i = 1; i < output.size(); i++)
    {
        auto emoteText = output.at(i).name;

        // test for match or match with colon at start for emotes like ":)"
        if (emoteText.compare(query, Qt::CaseInsensitive) == 0 ||
            emoteText.compare(":" + query, Qt::CaseInsensitive) == 0)
        {
            auto emote = output[i];
            output.erase(output.begin() + int(i));
            output.insert(output.begin(), emote);
            break;
        }
    }
}

}  // namespace chatterino
