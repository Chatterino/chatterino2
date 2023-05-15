#pragma once

#include "providers/autocomplete/AutocompleteEmoteSource.hpp"
#include "providers/autocomplete/AutocompleteStrategy.hpp"

namespace chatterino {

class ClassicAutocompleteEmoteStrategy
    : public AutocompleteStrategy<CompletionEmote>
{
    void apply(const std::vector<CompletionEmote> &items,
               std::vector<CompletionEmote> &output,
               const QString &query) const override;
};

}  // namespace chatterino
