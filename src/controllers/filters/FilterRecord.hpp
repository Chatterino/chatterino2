#pragma once

#include "util/RapidJsonSerializeQString.hpp"
#include "util/RapidjsonHelpers.hpp"

#include "controllers/filters/parser/FilterParser.hpp"

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
    {
        this->parser_ = new filterparser::FilterParser(filter);
    }

    FilterRecord(const QString &name, const QString &filter, const QUuid &id)
        : name_(name)
        , filter_(filter)
        , id_(id)
    {
        this->parser_ = new filterparser::FilterParser(filter);
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

    bool filter(const MessagePtr &message) const
    {
        return this->parser_->execute(message);
    }

private:
    QString name_;
    QString filter_;
    QUuid id_;

    filterparser::FilterParser *parser_;
};
}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::FilterRecord> {
    static rapidjson::Value get(const chatterino::FilterRecord &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "name", value.getName(), a);
        chatterino::rj::set(ret, "filter", value.getFilter(), a);
        chatterino::rj::set(ret, "id", value.getId().toString(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::FilterRecord> {
    static chatterino::FilterRecord get(const rapidjson::Value &value)
    {
        if (!value.IsObject())
        {
            return chatterino::FilterRecord(QString(), QString());
        }

        QString _name, _filter, _id;

        chatterino::rj::getSafe(value, "name", _name);
        chatterino::rj::getSafe(value, "filter", _filter);
        chatterino::rj::getSafe(value, "id", _id);

        return chatterino::FilterRecord(_name, _filter, QUuid::fromString(_id));
    }
};

}  // namespace pajlada
