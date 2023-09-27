#pragma once

#include "widgets/listview/GenericListModel.hpp"
#include "widgets/splits/InputCompletionItem.hpp"

#include <QStringList>

#include <memory>
#include <utility>
#include <vector>

namespace chatterino::completion {

/// @brief A Source represents a source for generating completion suggestions.
///
/// The source can be queried to update its suggestions and then write the completion
/// suggestions to a GenericListModel or QStringList depending on the consumer's
/// requirements.
///
/// For example, consider providing emotes for completion. The Source instance
/// initialized with every available emote in the channel (including  global
/// emotes). As the user updates their query by typing, the suggestions are
/// refined and the output model is updated.
class Source
{
public:
    virtual ~Source() = default;

    /// @brief Updates the internal completion suggestions for the given query
    /// @param query Query to complete against
    virtual void update(const QString &query) = 0;

    /// @brief Appends the internal completion suggestions to a GenericListModel
    /// @param model GenericListModel to add suggestions to
    /// @param maxCount Maximum number of suggestions. Zero indicates unlimited.
    virtual void addToListModel(GenericListModel &model,
                                size_t maxCount = 0) const = 0;

    /// @brief Appends the internal completion suggestions to a QStringList
    /// @param list QStringList to add suggestions to
    /// @param maxCount Maximum number of suggestions. Zero indicates unlimited.
    /// @param isFirstWord Whether the completion is the first word in the input
    virtual void addToStringList(QStringList &list, size_t maxCount = 0,
                                 bool isFirstWord = false) const = 0;
};

};  // namespace chatterino::completion
