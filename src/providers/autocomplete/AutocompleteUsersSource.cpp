#include "providers/autocomplete/AutocompleteUsersSource.hpp"

#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"

namespace chatterino {

namespace {

}  // namespace

AutocompleteUsersSource::AutocompleteUsersSource(
    const Channel *channel, ActionCallback callback,
    std::unique_ptr<AutocompleteUsersStrategy> strategy)
    : AutocompleteGenericSource({}, std::move(strategy))  // begin with no items
    , callback_(std::move(callback))
{
    this->initializeItems(channel);
}

void AutocompleteUsersSource::initializeItems(const Channel *channel)
{
    auto *tc = dynamic_cast<const TwitchChannel *>(channel);
    if (!tc)
    {
        return;
    }

    this->setItems(tc->accessChatters()->all());
}

std::unique_ptr<GenericListItem> AutocompleteUsersSource::mapListItem(
    const UsersAutocompleteItem &user) const
{
    return std::make_unique<InputCompletionItem>(nullptr, user.second,
                                                 this->callback_);
}

QString AutocompleteUsersSource::mapTabStringItem(
    const UsersAutocompleteItem &user, bool isFirstWord) const
{
    const auto userMention = formatUserMention(
        user.second, isFirstWord, getSettings()->mentionUsersWithComma);
    return "@" + userMention + " ";
}

}  // namespace chatterino
