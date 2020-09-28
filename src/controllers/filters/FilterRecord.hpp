#pragma once

#include "util/RapidJsonSerializeQString.hpp"
#include "util/RapidjsonHelpers.hpp"

#include "controllers/filters/parser/FilterParser.hpp"
#include "controllers/filters/parser/Types.hpp"

#include <QRegularExpression>
#include <QString>
#include <pajlada/serialize.hpp>

#include <memory>

namespace chatterino {

class FilterRecord
{
public:
    bool operator==(const FilterRecord &other) const
    {
        return std::tie(this->name_, this->filter_, this->id_) ==
               std::tie(other.name_, other.filter_, other.id_);
    }

    FilterRecord(const QString &name, const QString &filter)
        : name_(name)
        , filter_(filter)
        , id_(QUuid::createUuid())
        , parser_(std::make_unique<filterparser::FilterParser>(filter))
    {
    }

    FilterRecord(const QString &name, const QString &filter, const QUuid &id)
        : name_(name)
        , filter_(filter)
        , id_(id)
        , parser_(std::make_unique<filterparser::FilterParser>(filter))
    {
    }

    const QString &getName() const
    {
        return this->name_;
    }

    const QString &getFilter() const
    {
        return this->filter_;
    }

    const QUuid &getId() const
    {
        return this->id_;
    }

    bool valid() const
    {
        return this->parser_->valid();
    }

    bool filter(const MessagePtr &message) const
    {
        return this->parser_->execute(message);
    }

    bool filter(const filterparser::ContextMap &context) const
    {
        return this->parser_->execute(context);
    }

private:
    QString name_;
    QString filter_;
    QUuid id_;

    std::unique_ptr<filterparser::FilterParser> parser_;
};

using FilterRecordPtr = std::shared_ptr<FilterRecord>;

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::FilterRecordPtr> {
    static rapidjson::Value get(const chatterino::FilterRecordPtr &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "name", value->getName(), a);
        chatterino::rj::set(ret, "filter", value->getFilter(), a);
        chatterino::rj::set(ret, "id",
                            value->getId().toString(QUuid::WithoutBraces), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::FilterRecordPtr> {
    static chatterino::FilterRecordPtr get(const rapidjson::Value &value)
    {
        if (!value.IsObject())
        {
            return std::make_shared<chatterino::FilterRecord>(QString(),
                                                              QString());
        }

        QString _name, _filter, _id;

        chatterino::rj::getSafe(value, "name", _name);
        chatterino::rj::getSafe(value, "filter", _filter);
        chatterino::rj::getSafe(value, "id", _id);

        return std::make_shared<chatterino::FilterRecord>(
            _name, _filter, QUuid::fromString(_id));
    }
};

}  // namespace pajlada
