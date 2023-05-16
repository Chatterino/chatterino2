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

/// @brief AutocompleteGenericSource is a generic AutocompleteSource wrapping
/// a vector of items.
/// @tparam T The type of item being wrapped
template <typename T>
class AutocompleteGenericSource : public AutocompleteSource
{
public:
    using ListItemFunction =
        std::function<std::unique_ptr<GenericListItem>(const T &)>;

    /// @brief Creates a new AutocompleteGenericSource wrapping a vector of items
    /// @param items Items to wrap
    /// @param strategy AutocompleteStrategy to use when responding to a query
    AutocompleteGenericSource(std::vector<T> items,
                              std::unique_ptr<AutocompleteStrategy<T>> strategy)
        : items_(std::move(items))
        , strategy_(std::move(strategy))
    {
    }

    void update(const QString &query) override
    {
        this->output_.clear();
        if (this->strategy_)
        {
            this->strategy_->apply(this->items_, this->output_, query);
        }
    }

    void copyToListModel(GenericListModel &model,
                         size_t maxCount = 0) const override
    {
        model.clear();

        size_t i = 0;
        for (const auto &item : this->output_)
        {
            model.addItem(this->mapListItem(item));
            if (maxCount > 0 && i++ == maxCount)
            {
                break;
            }
        }
    }

    void copyToStringModel(QStringListModel &model, size_t maxCount = 0,
                           bool isFirstWord = false) const override
    {
        QStringList newData;

        size_t i = 0;
        for (const auto &item : this->output_)
        {
            newData.push_back(this->mapTabStringItem(item, isFirstWord));
            if (maxCount > 0 && i++ == maxCount)
            {
                break;
            }
        }

        model.setStringList(newData);
    }

protected:
    virtual std::unique_ptr<GenericListItem> mapListItem(
        const T &item) const = 0;
    virtual QString mapTabStringItem(const T &item, bool firstWord) const = 0;

    /// @brief Updates the wrapped items
    /// @param newItems New items to wrap
    void setItems(std::vector<T> newItems)
    {
        this->items_ = std::move(newItems);
    }

    /// @brief Updates the wrapped items through an iterator pair
    /// @tparam Iterator Iterator type, accepted by std::vector
    /// @param begin Begin iterator
    /// @param end End iterator
    template <typename Iterator>
    void setItems(Iterator begin, Iterator end)
    {
        this->items_ = std::vector<T>(begin, end);
    }

private:
    std::vector<T> items_;
    // TODO: vector of pointers? must not invalidate by items_ change (clear right away)
    std::vector<T> output_;

    std::unique_ptr<AutocompleteStrategy<T>> strategy_;
};

};  // namespace chatterino
