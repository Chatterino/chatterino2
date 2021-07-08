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
    }

    const QString &getName() const
    {
        return this->name_;
    }
    const QString &getReplace() const
    {
        return this->replace_;
    }

private:
    QString name_;
    QString replace_;
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::AliasesName> {
    static rapidjson::Value get(const chatterino::AliasesName &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "name", value.getName(), a);
        chatterino::rj::set(ret, "replace", value.getReplace(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::AliasesName> {
    static chatterino::AliasesName get(const rapidjson::Value &value,
                                       bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return chatterino::AliasesName(QString(), QString());
        }

        QString _name;
        QString _replace;

        chatterino::rj::getSafe(value, "name", _name);
        chatterino::rj::getSafe(value, "replace", _replace);

        return chatterino::AliasesName(_name, _replace);
    }
};

}  // namespace pajlada
