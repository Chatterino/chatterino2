#include "controllers/filters/FilterRecord.hpp"

namespace chatterino {

FilterRecord::FilterRecord(const QString &name, const QString &filter)
    : name_(name)
    , filter_(filter)
    , id_(QUuid::createUuid())
    , parser_(std::make_unique<filterparser::FilterParser>(filter))
{
}

FilterRecord::FilterRecord(const QString &name, const QString &filter,
                           const QUuid &id)
    : name_(name)
    , filter_(filter)
    , id_(id)
    , parser_(std::make_unique<filterparser::FilterParser>(filter))
{
}

const QString &FilterRecord::getName() const
{
    return this->name_;
}

const QString &FilterRecord::getFilter() const
{
    return this->filter_;
}

const QUuid &FilterRecord::getId() const
{
    return this->id_;
}

bool FilterRecord::valid() const
{
    return this->parser_->valid();
}

bool FilterRecord::filter(const filterparser::ContextMap &context) const
{
    return this->parser_->execute(context);
}

bool FilterRecord::operator==(const FilterRecord &other) const
{
    return std::tie(this->name_, this->filter_, this->id_) ==
           std::tie(other.name_, other.filter_, other.id_);
}

}  // namespace chatterino
