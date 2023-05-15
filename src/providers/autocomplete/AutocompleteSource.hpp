#pragma once

#include "providers/autocomplete/AutocompleteStrategy.hpp"
#include "widgets/listview/GenericListModel.hpp"
#include "widgets/splits/InputCompletionItem.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace chatterino {

class AutocompleteSource
{
public:
    virtual ~AutocompleteSource() = default;

    virtual void update(const QString &query) = 0;
    virtual void copyToModel(GenericListModel &model,
                             size_t maxCount = 0) const = 0;
};

template <typename T>
class AutocompleteGenericSource : public AutocompleteSource
{
public:
    using ListItemFunction =
        std::function<std::unique_ptr<GenericListItem>(const T &)>;

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

    void copyToModel(GenericListModel &model,
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

protected:
    virtual std::unique_ptr<GenericListItem> mapListItem(
        const T &item) const = 0;

    void setItems(std::vector<T> newItems)
    {
        this->items_ = std::move(newItems);
    }

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
