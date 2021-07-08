#pragma once

#include "controllers/accounts/AccountController.hpp"

#include "util/RapidJsonSerializeQString.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <QString>
#include <pajlada/serialize.hpp>

#include <memory>

namespace chatterino {

class AliasesName
{
public:
    AliasesName(const QString &name, const QString &replace)
        : name_(name)
        , replace_(replace)
    {
        id_ = "";
    }
    AliasesName(const QString &name, const QString &replace, const QString &id)
        : name_(name)
        , replace_(replace)
        , id_(id)
    {
    }

    const QString &getName() const
    {
        return this->name_;
    }
    const QString &getId() const
    {
        return this->id_;
    }
    const QString &getReplace() const
    {
        return this->replace_;
    }
    void setId(QString id)
    {
        this->id_ = id;
    }

private:
    QString name_;
    QString id_;
    QString replace_;
};

using AliasesNamePtr = std::shared_ptr<AliasesName>;

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::AliasesNamePtr> {
    static rapidjson::Value get(const chatterino::AliasesNamePtr &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "name", value->getName(), a);
        chatterino::rj::set(ret, "id", value->getId(), a);
        chatterino::rj::set(ret, "replace", value->getReplace(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::AliasesNamePtr> {
    static chatterino::AliasesNamePtr get(const rapidjson::Value &value,
                                          bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return std::make_shared<chatterino::AliasesName>(
                QString(), QString(), QString());
        }

        QString _name;
        QString _id;
        QString _replace;

        chatterino::rj::getSafe(value, "name", _name);
        chatterino::rj::getSafe(value, "id", _id);
        chatterino::rj::getSafe(value, "replace", _replace);

        return std::make_shared<chatterino::AliasesName>(_name, _replace, _id);
    }
};

}  // namespace pajlada
