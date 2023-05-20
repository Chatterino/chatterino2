#pragma once

#include "providers/autocomplete/AutocompleteSources.hpp"
#include "providers/autocomplete/AutocompleteStrategy.hpp"

namespace chatterino {

//// Emote strategies

class ClassicAutocompleteEmoteStrategy
    : public AutocompleteStrategy<CompletionEmote>
{
    void apply(const std::vector<CompletionEmote> &items,
               std::vector<CompletionEmote> &output,
               const QString &query) const override;
};

class ClassicTabAutocompleteEmoteStrategy
    : public AutocompleteStrategy<CompletionEmote>
{
    void apply(const std::vector<CompletionEmote> &items,
               std::vector<CompletionEmote> &output,
               const QString &query) const override;
};

//// User strategies

class ClassicAutocompleteUserStrategy
    : public AutocompleteStrategy<UsersAutocompleteItem>
{
    void apply(const std::vector<UsersAutocompleteItem> &items,
               std::vector<UsersAutocompleteItem> &output,
               const QString &query) const override;
};

//// Command strategies

class AutocompleteCommandStrategy
    : public AutocompleteStrategy<CompletionCommand>
{
public:
    AutocompleteCommandStrategy(bool startsWithOnly);

    void apply(const std::vector<CompletionCommand> &items,
               std::vector<CompletionCommand> &output,
               const QString &query) const override;

private:
    bool startsWithOnly_;
};

}  // namespace chatterino
