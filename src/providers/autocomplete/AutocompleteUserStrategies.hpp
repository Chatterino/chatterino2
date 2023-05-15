#pragma once

#include "providers/autocomplete/AutocompleteStrategy.hpp"
#include "providers/autocomplete/AutocompleteUsersSource.hpp"

namespace chatterino {

class ClassicAutocompleteUserStrategy
    : public AutocompleteStrategy<UsersAutocompleteItem>
{
    void apply(const std::vector<UsersAutocompleteItem> &items,
               std::vector<UsersAutocompleteItem> &output,
               const QString &query) const override;
};

}  // namespace chatterino
