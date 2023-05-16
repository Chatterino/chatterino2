#pragma once

#include "common/Channel.hpp"
#include "providers/autocomplete/AutocompleteSource.hpp"

#include <QString>

#include <functional>
#include <memory>
#include <utility>

namespace chatterino {

using UsersAutocompleteItem = std::pair<QString, QString>;

class AutocompleteUsersSource
    : public AutocompleteGenericSource<UsersAutocompleteItem>
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using AutocompleteUsersStrategy =
        AutocompleteStrategy<UsersAutocompleteItem>;

    AutocompleteUsersSource(
        ChannelPtr channel, ActionCallback callback,
        std::unique_ptr<AutocompleteUsersStrategy> strategy);

protected:
    std::unique_ptr<GenericListItem> mapListItem(
        const UsersAutocompleteItem &user) const override;

    QString mapTabStringItem(const UsersAutocompleteItem &user,
                             bool isFirstWord) const override;

private:
    void initializeItems(ChannelPtr channel);

    ActionCallback callback_;
};

}  // namespace chatterino
