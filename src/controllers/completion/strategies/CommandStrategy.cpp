#include "controllers/completion/strategies/CommandStrategy.hpp"

namespace chatterino::completion {

CommandStrategy::CommandStrategy(bool startsWithOnly)
    : startsWithOnly_(startsWithOnly)
{
}

void CommandStrategy::apply(const std::vector<CommandItem> &items,
                            std::vector<CommandItem> &output,
                            const QString &query) const
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
