#pragma once

#include "widgets/listview/GenericListModel.hpp"

#include <QObject>
#include <QString>

namespace chatterino {

namespace completion {
    class Source;
}  // namespace completion

/// @brief Represents the kind of completion occurring
enum class CompletionKind {
    Emote,
    User,
};

/// @brief CompletionModel is a GenericListModel intended to provide completion
/// suggestions to an InputCompletionPopup. The popup can determine the appropriate
/// source based on the current input and the user's preferences.
class CompletionModel final : public GenericListModel
{
public:
    explicit CompletionModel(QObject *parent);

    /// @brief Sets the Source for subsequent queries
    /// @param source Source to use
    void setSource(std::unique_ptr<completion::Source> source);

    /// @return Whether the model has a source set
    bool hasSource() const;

    /// @brief Updates the model based on the completion query
    /// @param query Completion query
    /// @param maxCount Maximum number of results. Zero indicates unlimited.
    void updateResults(const QString &query, size_t maxCount = 0);

private:
    std::unique_ptr<completion::Source> source_{};
};

};  // namespace chatterino
