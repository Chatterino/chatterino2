#pragma once

#include "controllers/completion/sources/Source.hpp"

#include <QObject>
#include <QString>
#include <QStringListModel>

#include <optional>

namespace chatterino {

class Channel;

/// @brief TabCompletionModel is a QStringListModel intended to provide tab
/// completion to a ResizingTextInput. The model automatically selects a completion
/// source based on the current query before updating the results.
class TabCompletionModel : public QStringListModel
{
public:
    /// @brief Initializes a new TabCompletionModel bound to a Channel.
    /// The reference to the Channel must live as long as the TabCompletionModel.
    /// @param channel Channel reference
    /// @param parent Model parent
    explicit TabCompletionModel(Channel &channel, QObject *parent);

    /// @brief Updates the model based on the completion query
    /// @param query Completion query
    /// @param fullTextContent Full text of the input, used by plugins for contextual completion
    /// @param cursorPosition Number of characters behind the cursor from the
    /// beginning of fullTextContent, also used by plugins
    /// @param isFirstWord Whether the completion is the first word in the input
    void updateResults(const QString &query, const QString &fullTextContent,
                       int cursorPosition, bool isFirstWord = false);

private:
    enum class SourceKind {
        // Known to be an emote, i.e. started with :
        Emote,
        // Known to be a username, i.e. started with @
        User,
        // Known to be a command, i.e. started with / or .
        Command,
        // Emote or command without : or / .
        EmoteCommand,
        // Emote, user, or command without :, @, / .
        EmoteUserCommand
    };

    /// @brief Updates the internal completion source based on the current query.
    /// The completion source will only change if the deduced completion kind
    /// changes (see deduceSourceKind).
    /// @param query Completion query
    void updateSourceFromQuery(const QString &query);

    /// @brief Attempts to deduce the source kind from the current query. If the
    /// bound Channel is not a TwitchChannel or if the query is too short, no
    /// query type will be deduced to prevent completions.
    /// @param query Completion query
    /// @return An optional SourceKind deduced from the query
    std::optional<SourceKind> deduceSourceKind(const QString &query) const;

    std::unique_ptr<completion::Source> buildSource(SourceKind kind) const;

    std::unique_ptr<completion::Source> buildEmoteSource() const;
    std::unique_ptr<completion::Source> buildUserSource(bool prependAt) const;
    std::unique_ptr<completion::Source> buildCommandSource() const;

    Channel &channel_;
    std::unique_ptr<completion::Source> source_{};
};

}  // namespace chatterino
