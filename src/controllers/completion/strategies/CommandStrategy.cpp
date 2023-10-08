#include "controllers/completion/strategies/CommandStrategy.hpp"

namespace chatterino::completion {

QString normalizeQuery(const QString &query)
{
    if (query.startsWith('/') || query.startsWith('.'))
    {
        return query.mid(1);
    }

    return query;
}

CommandStrategy::CommandStrategy(bool startsWithOnly)
    : startsWithOnly_(startsWithOnly)
{
}

void CommandStrategy::apply(const std::vector<CommandItem> &items,
                            std::vector<CommandItem> &output,
                            const QString &query) const
{
    QString normalizedQuery = normalizeQuery(query);

    if (startsWithOnly_)
    {
        std::copy_if(items.begin(), items.end(),
                     std::back_insert_iterator(output),
                     [&normalizedQuery](const CommandItem &item) {
                         return item.name.startsWith(normalizedQuery,
                                                     Qt::CaseInsensitive);
                     });
    }
    else
    {
        std::copy_if(
            items.begin(), items.end(), std::back_insert_iterator(output),
            [&normalizedQuery](const CommandItem &item) {
                return item.name.contains(normalizedQuery, Qt::CaseInsensitive);
            });
    }
};

}  // namespace chatterino::completion
