#include "controllers/completion/strategies/ClassicUserStrategy.hpp"

namespace chatterino::completion {

void ClassicUserStrategy::apply(const std::vector<UserItem> &items,
                                std::vector<UserItem> &output,
                                const QString &query) const
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
}  // namespace chatterino::completion
