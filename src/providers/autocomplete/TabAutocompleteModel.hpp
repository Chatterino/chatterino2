#pragma once

#include "providers/autocomplete/AutocompleteSource.hpp"

#include <boost/optional.hpp>
#include <QObject>
#include <QString>
#include <QStringListModel>

namespace chatterino {

class Channel;

/// @brief TabAutocompleteModel is a QStringListModel intended to provide tab
/// completion to a ResizingTextInput. The model automatically selects a AutocompleteSource
/// based on the current query before updating the results.
class TabAutocompleteModel : public QStringListModel
{
public:
    /// @brief Initializes a new TabAutocompleteModel bound to a Channel.
    /// The reference to the Channel must live as long as the TabAutocompleteModel.
    /// @param channel Channel reference
    /// @param parent Model parent
    TabAutocompleteModel(Channel &channel, QObject *parent = nullptr);

    /// @brief Updates the model based on the autocomplete query
    /// @param query Autocomplete query
    /// @param isFirstWord Whether the completion is the first word in the input
    void updateResults(const QString &query, bool isFirstWord = false);

private:
    enum class SourceKind { Emote, User, Command, EmoteAndUser };

    /// @brief Updates the internal AutocompleteSource based on the current query.
    /// The AutocompleteSource will only change if the deduced completion kind
    /// changes (see deduceSourceKind).
    /// @param query Autocomplete query
    void updateSourceFromQuery(const QString &query);

    /// @brief Attempts to deduce the source kind from the current query. If the
    /// bound Channel is not a TwitchChannel or if the query is too short, no
    /// query type will be deduced to prevent completions.
    /// @param query Autocomplete query
    /// @return An optional SourceKind deduced from the query
    boost::optional<SourceKind> deduceSourceKind(const QString &query) const;

    std::unique_ptr<AutocompleteSource> buildSource(SourceKind kind) const;

    Channel &channel_;
    std::unique_ptr<AutocompleteSource> source_{};
    boost::optional<SourceKind> sourceKind_{};
};

}  // namespace chatterino
