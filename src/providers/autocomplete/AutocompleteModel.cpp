#include "providers/autocomplete/AutocompleteModel.hpp"

namespace chatterino {

AutocompleteModel::AutocompleteModel(QObject *parent)
    : GenericListModel(parent)
{
}

void AutocompleteModel::setSource(std::unique_ptr<AutocompleteSource> source)
{
    this->source_ = std::move(source);
}

bool AutocompleteModel::hasSource() const
{
    return this->source_ != nullptr;
}

void AutocompleteModel::updateResults(const QString &query, size_t maxCount)
{
    if (this->source_)
    {
        this->source_->update(query);

        // Copy results to this model
        this->clear();
        this->source_->addToListModel(*this, maxCount);
    }
}

}  // namespace chatterino