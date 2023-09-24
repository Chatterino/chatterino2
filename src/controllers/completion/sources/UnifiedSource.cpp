#include "controllers/completion/sources/UnifiedSource.hpp"

namespace chatterino::completion {

UnifiedSource::UnifiedSource(
    const Channel *channel,
    std::unique_ptr<EmoteSource::EmoteStrategy> emoteStrategy,
    std::unique_ptr<UserSource::UserStrategy> userStrategy,
    ActionCallback callback)
    : emoteSource_(channel, std::move(emoteStrategy), callback)
    , usersSource_(channel, std::move(userStrategy), callback,
                   false)  // disable adding @ to front
{
}

void UnifiedSource::update(const QString &query)
{
    this->emoteSource_.update(query);
    this->usersSource_.update(query);
}

void UnifiedSource::addToListModel(GenericListModel &model,
                                   size_t maxCount) const
{
    if (maxCount == 0)
    {
        this->emoteSource_.addToListModel(model, 0);
        this->usersSource_.addToListModel(model, 0);
        return;
    }

    // Otherwise, make sure to only add maxCount elements in total. We prioritize
    // accepting results from the emote source before the users source (arbitrarily).

    int startingSize = model.rowCount();

    // Add up to maxCount elements
    this->emoteSource_.addToListModel(model, maxCount);

    int used = model.rowCount() - startingSize;
    if (used >= maxCount)
    {
        // Used up our limit on emotes
        return;
    }

    // Only add maxCount - used to ensure the total added doesn't exceed maxCount
    this->usersSource_.addToListModel(model, maxCount - used);
}

void UnifiedSource::addToStringList(QStringList &list, size_t maxCount,
                                    bool isFirstWord) const
{
    if (maxCount == 0)
    {
        this->emoteSource_.addToStringList(list, 0, isFirstWord);
        this->usersSource_.addToStringList(list, 0, isFirstWord);
        return;
    }

    // Otherwise, make sure to only add maxCount elements in total. We prioritize
    // accepting results from the emote source before the users source (arbitrarily).

    int startingSize = list.size();

    // Add up to maxCount elements
    this->emoteSource_.addToStringList(list, maxCount, isFirstWord);

    int used = list.size() - startingSize;
    if (used >= maxCount)
    {
        // Used up our limit on emotes
        return;
    }

    // Only add maxCount - used to ensure the total added doesn't exceed maxCount
    this->usersSource_.addToStringList(list, maxCount - used, isFirstWord);
}

}  // namespace chatterino::completion
