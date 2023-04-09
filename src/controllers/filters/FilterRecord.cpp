#include "controllers/filters/FilterRecord.hpp"

#include "controllers/filters/lang/Filter.hpp"

namespace chatterino {

static std::unique_ptr<filters::Filter> buildFilter(const QString &filterText)
{
    using namespace filters;
    auto result = Filter::fromString(filterText);
    if (std::holds_alternative<Filter>(result))
    {
        auto filter =
            std::make_unique<Filter>(std::move(std::get<Filter>(result)));

        if (filter->returnType() != Type::Bool)
        {
            // Only accept Bool results
            return nullptr;
        }

        return filter;
    }

    return nullptr;
}

FilterRecord::FilterRecord(QString name, QString filter)
    : FilterRecord(std::move(name), std::move(filter), QUuid::createUuid())
{
}

FilterRecord::FilterRecord(QString name, QString filter, const QUuid &id)
    : name_(std::move(name))
    , filterText_(std::move(filter))
    , id_(id)
    , filter_(buildFilter(this->filterText_))
{
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
