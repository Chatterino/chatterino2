#include "FilterRecord.hpp"

#include "controllers/filters/lang/Filter.hpp"

namespace chatterino {

FilterRecord::FilterRecord(const QString &name, const QString &filter)
    : FilterRecord(name, filter, QUuid::createUuid())
{
}

FilterRecord::FilterRecord(const QString &name, const QString &filter,
                           const QUuid &id)
    : name_(name)
    , filterText_(filter)
    , id_(id)
{
    using namespace filters;
    auto result = Filter::fromString(filter);
    if (std::holds_alternative<Filter>(result))
    {
        this->filter_ =
            std::make_unique<Filter>(std::move(std::get<Filter>(result)));

        if (this->filter_->returnType() != Type::Bool)
        {
            // Only accept Bool results
            this->filter_ = nullptr;
        }
    }
    else
    {
        this->filter_ = nullptr;
    }
}

const QString &FilterRecord::getName() const
{
    return this->name_;
}

const QString &FilterRecord::getFilter() const
{
    return this->filterText_;
}

const QUuid &FilterRecord::getId() const
{
    return this->id_;
}

bool FilterRecord::valid() const
{
    return this->filter_ != nullptr;
}

bool FilterRecord::filter(const filters::ContextMap &context) const
{
    assert(this->valid());
    return this->filter_->execute(context).toBool();
}

bool FilterRecord::operator==(const FilterRecord &other) const
{
    return std::tie(this->name_, this->filter_, this->id_) ==
           std::tie(other.name_, other.filter_, other.id_);
}

}  // namespace chatterino
