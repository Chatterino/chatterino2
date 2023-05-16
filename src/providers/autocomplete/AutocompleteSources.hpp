#pragma once

#include "common/Channel.hpp"
#include "messages/Emote.hpp"
#include "providers/autocomplete/AutocompleteSource.hpp"

#include <QString>

#include <functional>
#include <memory>
#include <utility>

namespace chatterino {

//// AutocompleteEmoteSource

struct CompletionEmote {
    /// Emote image to show in input popup
    EmotePtr emote{};
    /// Name to check completion queries against
    QString searchName{};
    /// Name to insert into split input upon tab completing
    QString tabCompletionName{};
    /// Display name within input popup
    QString displayName{};
    /// Emote provider name for input popup
    QString providerName{};
};

class AutocompleteEmoteSource
    : public AutocompleteGenericSource<CompletionEmote>
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using AutocompleteEmoteStrategy = AutocompleteStrategy<CompletionEmote>;

    /// @brief Initializes a source for CompletionEmotes from the given channel
    /// @param channel Channel to initialize emotes from
    /// @param callback ActionCallback to invoke upon InputCompletionItem selection.
    /// See InputCompletionItem::action(). Can be nullptr.
    /// @param strategy AutocompleteStrategy to apply
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

//// AutocompleteUsersSource

using UsersAutocompleteItem = std::pair<QString, QString>;

class AutocompleteUsersSource
    : public AutocompleteGenericSource<UsersAutocompleteItem>
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using AutocompleteUsersStrategy =
        AutocompleteStrategy<UsersAutocompleteItem>;

    /// @brief Initializes a source for UsersAutocompleteItems from the given
    /// channel.
    /// @param channel Channel to initialize emotes from. Must be a TwitchChannel
    /// or completion is a no-op.
    /// @param callback ActionCallback to invoke upon InputCompletionItem selection.
    /// See InputCompletionItem::action(). Can be nullptr.
    /// @param strategy AutocompleteStrategy to apply
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

//// AutocompleteCommandsSource

struct CompleteCommand {
    QString name{};
    QChar prefix{};
};

class AutocompleteCommandsSource
    : public AutocompleteGenericSource<CompleteCommand>
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using AutocompleteCommandStrategy = AutocompleteStrategy<CompleteCommand>;

    /// @brief Initializes a source for CompleteCommands.
    /// @param callback ActionCallback to invoke upon InputCompletionItem selection.
    /// See InputCompletionItem::action(). Can be nullptr.
    /// @param strategy AutocompleteStrategy to apply
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
