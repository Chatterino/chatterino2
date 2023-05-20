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

class AutocompleteEmoteSource : public AutocompleteSource
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using AutocompleteEmoteStrategy = AutocompleteStrategy<CompletionEmote>;

    /// @brief Initializes a source for CompletionEmotes from the given channel
    /// @param channel Channel to initialize emotes from
    /// @param strategy AutocompleteStrategy to apply
    /// @param callback ActionCallback to invoke upon InputCompletionItem selection.
    /// See InputCompletionItem::action(). Can be nullptr.
    AutocompleteEmoteSource(const Channel &channel,
                            std::unique_ptr<AutocompleteEmoteStrategy> strategy,
                            ActionCallback callback = nullptr);

    void update(const QString &query) override;
    void copyToListModel(GenericListModel &model,
                         size_t maxCount = 0) const override;
    void copyToStringModel(QStringListModel &model, size_t maxCount = 0,
                           bool isFirstWord = false) const override;

private:
    void initializeFromChannel(const Channel *channel);

    std::unique_ptr<AutocompleteEmoteStrategy> strategy_;
    ActionCallback callback_;

    std::vector<CompletionEmote> items_{};
    std::vector<CompletionEmote> output_{};
};

//// AutocompleteUsersSource

using UsersAutocompleteItem = std::pair<QString, QString>;

class AutocompleteUsersSource : public AutocompleteSource
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using AutocompleteUsersStrategy =
        AutocompleteStrategy<UsersAutocompleteItem>;

    /// @brief Initializes a source for UsersAutocompleteItems from the given
    /// channel.
    /// @param channel Channel to initialize emotes from. Must be a TwitchChannel
    /// or completion is a no-op.
    /// @param strategy AutocompleteStrategy to apply
    /// @param callback ActionCallback to invoke upon InputCompletionItem selection.
    /// See InputCompletionItem::action(). Can be nullptr.
    AutocompleteUsersSource(const Channel &channel,
                            std::unique_ptr<AutocompleteUsersStrategy> strategy,
                            ActionCallback callback = nullptr);

    void update(const QString &query) override;
    void copyToListModel(GenericListModel &model,
                         size_t maxCount = 0) const override;
    void copyToStringModel(QStringListModel &model, size_t maxCount = 0,
                           bool isFirstWord = false) const override;

private:
    void initializeFromChannel(const Channel *channel);

    std::unique_ptr<AutocompleteUsersStrategy> strategy_;
    ActionCallback callback_;

    std::vector<UsersAutocompleteItem> items_{};
    std::vector<UsersAutocompleteItem> output_{};
};

//// AutocompleteCommandsSource

struct CompletionCommand {
    QString name{};
    QChar prefix{};
};

class AutocompleteCommandsSource : public AutocompleteSource
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using AutocompleteCommandStrategy = AutocompleteStrategy<CompletionCommand>;

    /// @brief Initializes a source for CompleteCommands.
    /// @param strategy AutocompleteStrategy to apply
    /// @param callback ActionCallback to invoke upon InputCompletionItem selection.
    /// See InputCompletionItem::action(). Can be nullptr.
    AutocompleteCommandsSource(
        std::unique_ptr<AutocompleteCommandStrategy> strategy,
        ActionCallback callback = nullptr);

    void update(const QString &query) override;
    void copyToListModel(GenericListModel &model,
                         size_t maxCount = 0) const override;
    void copyToStringModel(QStringListModel &model, size_t maxCount = 0,
                           bool isFirstWord = false) const override;

private:
    void initializeItems();

    std::unique_ptr<AutocompleteCommandStrategy> strategy_;
    ActionCallback callback_;

    std::vector<CompletionCommand> items_{};
    std::vector<CompletionCommand> output_{};
};

}  // namespace chatterino
