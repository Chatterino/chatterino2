#include "providers/autocomplete/AutocompleteCommandStrategies.hpp"

namespace chatterino {

AutocompleteCommandStrategy::AutocompleteCommandStrategy(bool startsWithOnly)
    : startsWithOnly_(startsWithOnly)
{
}

void AutocompleteCommandStrategy::apply(
    const std::vector<CompleteCommand> &items,
    std::vector<CompleteCommand> &output, const QString &query) const
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
                     [&normalizedQuery](const CompleteCommand &item) {
                         return item.name.startsWith(normalizedQuery,
                                                     Qt::CaseInsensitive);
                     });
    }
    else
    {
        std::copy_if(
            items.begin(), items.end(), std::back_insert_iterator(output),
            [&normalizedQuery](const CompleteCommand &item) {
                return item.name.contains(normalizedQuery, Qt::CaseInsensitive);
            });
    }
};

}  // namespace chatterino
