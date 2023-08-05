#pragma once

#include "providers/autocomplete/AutocompleteSource.hpp"
#include "widgets/listview/GenericListModel.hpp"

#include <QObject>
#include <QString>

namespace chatterino {

/// @brief Represents the kind of completion occurring
enum class CompletionKind {
    Emote,
    User,
};

/// @brief AutocompleteModel is a GenericListModel intended to provide completion
/// suggestions to an InputCompletionPopup. The popup can determine the appropriate
/// source based on the current input and the user's preferences.
class AutocompleteModel : public GenericListModel
{
public:
    AutocompleteModel(QObject *parent = nullptr);

    /// @brief Sets the AutocompleteSource for subsequent queries
    /// @param source AutocompleteSource to use
    void setSource(std::unique_ptr<AutocompleteSource> source);

    /// @return Whether the model has a source set
    bool hasSource() const;

    /// @brief Updates the model based on the autocomplete query
    /// @param query Autocomplete query
    /// @param maxCount Maximum number of results. Zero indicates unlimited.
    void updateResults(const QString &query, size_t maxCount = 0);

private:
    std::unique_ptr<AutocompleteSource> source_{};
};

};  // namespace chatterino
