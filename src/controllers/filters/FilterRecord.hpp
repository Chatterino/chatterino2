#pragma once

#include "controllers/filters/lang/Filter.hpp"
#include "util/RapidjsonHelpers.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <pajlada/serialize.hpp>
#include <QRegularExpression>
#include <QString>
#include <QUuid>

#include <memory>

namespace chatterino {

class FilterRecord
{
public:
    FilterRecord(QString name, QString filter);

    FilterRecord(QString name, QString filter, const QUuid &id);

    const QString &getName() const;

    const QString &getFilter() const;

    const QUuid &getId() const;

    bool valid() const;

    bool filter(const filters::ContextMap &context) const;

    bool operator==(const FilterRecord &other) const;

private:
    const QString name_;
    const QString filterText_;
    const QUuid id_;

    const std::unique_ptr<filters::Filter> filter_;
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
    static chatterino::FilterRecordPtr get(const rapidjson::Value &value,
                                           bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
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
