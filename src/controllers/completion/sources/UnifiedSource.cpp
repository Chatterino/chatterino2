#include "controllers/completion/sources/UnifiedSource.hpp"

namespace chatterino::completion {

UnifiedSource::UnifiedSource(std::vector<std::unique_ptr<Source>> sources)
    : sources_(std::move(sources))
{
}

void UnifiedSource::update(const QString &query)
{
    // Update all sources
    for (const auto &source : this->sources_)
    {
        source->update(query);
    }
}

void UnifiedSource::addToListModel(GenericListModel &model,
                                   size_t maxCount) const
{
    if (maxCount == 0)
    {
        for (const auto &source : this->sources_)
        {
            source->addToListModel(model, 0);
        }
        return;
    }

    // Make sure to only add maxCount elements in total.
    int startingSize = model.rowCount();
    int used = 0;

    for (const auto &source : this->sources_)
    {
        source->addToListModel(model, maxCount - used);
        // Calculate how many items have been added so far
        used = model.rowCount() - startingSize;
        if (used >= static_cast<int>(maxCount))
        {
            // Used up all of limit
            break;
        }
    }
}

void UnifiedSource::addToStringList(QStringList &list, size_t maxCount,
                                    bool isFirstWord) const
{
    if (maxCount == 0)
    {
        for (const auto &source : this->sources_)
        {
            source->addToStringList(list, 0, isFirstWord);
        }
        return;
    }

    // Make sure to only add maxCount elements in total.
    auto startingSize = list.size();
    QStringList::size_type used = 0;

    for (const auto &source : this->sources_)
    {
        source->addToStringList(list, maxCount - used, isFirstWord);
        // Calculate how many items have been added so far
        used = list.size() - startingSize;
        if (used >= static_cast<QStringList::size_type>(maxCount))
        {
            // Used up all of limit
            break;
        }
    }
}

}  // namespace chatterino::completion
