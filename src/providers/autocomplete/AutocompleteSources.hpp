#pragma once

#include "common/Channel.hpp"
#include "messages/Emote.hpp"
#include "providers/autocomplete/AutocompleteSource.hpp"

#include <QString>

#include <functional>
#include <memory>
#include <utility>

namespace chatterino {

struct CompletionEmote {
    // emote image to show in input popup
    EmotePtr emote;
    // name to check completion queries against
    QString searchName;
    // name to insert into split input upon tab completing
    QString tabCompletionName;
    // display name within input popup
    QString displayName;
    // emote provider name for input popup
    QString providerName;
};

class AutocompleteEmoteSource
    : public AutocompleteGenericSource<CompletionEmote>
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using AutocompleteEmoteStrategy = AutocompleteStrategy<CompletionEmote>;

    AutocompleteEmoteSource(
        const Channel *channel, ActionCallback callback,
        std::unique_ptr<AutocompleteEmoteStrategy> strategy);

protected:
    std::unique_ptr<GenericListItem> mapListItem(
        const CompletionEmote &emote) const override;

    QString mapTabStringItem(const CompletionEmote &emote,
                             bool isFirstWord) const override;

private:
    void initializeItems(const Channel *channel);

    ActionCallback callback_;
};

using UsersAutocompleteItem = std::pair<QString, QString>;

class AutocompleteUsersSource
    : public AutocompleteGenericSource<UsersAutocompleteItem>
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using AutocompleteUsersStrategy =
        AutocompleteStrategy<UsersAutocompleteItem>;

    AutocompleteUsersSource(
        const Channel *channel, ActionCallback callback,
        std::unique_ptr<AutocompleteUsersStrategy> strategy);

protected:
    std::unique_ptr<GenericListItem> mapListItem(
        const UsersAutocompleteItem &user) const override;

    QString mapTabStringItem(const UsersAutocompleteItem &user,
                             bool isFirstWord) const override;

private:
    void initializeItems(const Channel *channel);

    ActionCallback callback_;
};

struct CompleteCommand {
    QString name;
    QChar prefix;
};

class AutocompleteCommandsSource
    : public AutocompleteGenericSource<CompleteCommand>
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using AutocompleteCommandStrategy = AutocompleteStrategy<CompleteCommand>;

    AutocompleteCommandsSource(
        ActionCallback callback,
        std::unique_ptr<AutocompleteCommandStrategy> strategy);

protected:
    std::unique_ptr<GenericListItem> mapListItem(
        const CompleteCommand &command) const override;

    QString mapTabStringItem(const CompleteCommand &command,
                             bool isFirstWord) const override;

private:
    void initializeItems();

    ActionCallback callback_;
};

}  // namespace chatterino
