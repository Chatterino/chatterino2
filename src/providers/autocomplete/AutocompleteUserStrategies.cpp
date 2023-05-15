#include "providers/autocomplete/AutocompleteUserStrategies.hpp"

namespace chatterino {

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

}  // namespace chatterino
