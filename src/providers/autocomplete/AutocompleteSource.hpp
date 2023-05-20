#pragma once

#include "providers/autocomplete/AutocompleteStrategy.hpp"
#include "widgets/listview/GenericListModel.hpp"
#include "widgets/splits/InputCompletionItem.hpp"

#include <QStringListModel>

#include <memory>
#include <utility>
#include <vector>

namespace chatterino {

/// @brief An AutocompleteSource represents a source for generating autocomplete suggestions.
///
/// The source can be queried to update its suggestions and then write the completion
/// suggestions to a GenericListModel or QStringListModel depending on the consumer's
/// requirements.
///
/// For example, consider providing emotes for autocomplete. The AutocompleteSource
/// instance is initialized with every available emote in the channel (including
/// global emotes). As the user updates their query by typing, the suggestions are
/// refined and the output model is updated.
class AutocompleteSource
{
public:
    virtual ~AutocompleteSource() = default;

    /// @brief Updates the internal completion suggestions for the given query
    /// @param query Query to complete against
    virtual void update(const QString &query) = 0;

    /// @brief Copies the internal completion suggestions to a GenericListModel
    /// @param model GenericListModel to copy suggestions to
    /// @param maxCount Maximum number of suggestions. Zero indicates unlimited.
    virtual void copyToListModel(GenericListModel &model,
                                 size_t maxCount = 0) const = 0;

    /// @brief Copies the internal completion suggestions to a QStringListModel
    /// @param model QStringListModel to copy suggestions to
    /// @param maxCount Maximum number of suggestions. Zero indicates unlimited.
    /// @param isFirstWord Whether the completion is the first word in the input
    virtual void copyToStringModel(QStringListModel &model, size_t maxCount = 0,
                                   bool isFirstWord = false) const = 0;
};

};  // namespace chatterino
