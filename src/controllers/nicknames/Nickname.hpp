#pragma once

#include "controllers/accounts/AccountController.hpp"

#include "util/RapidJsonSerializeQString.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <QString>
#include <pajlada/serialize.hpp>

#include <memory>

namespace {

const QString REGEX_START_BOUNDARY("(\\b|\\s|^)");
const QString REGEX_END_BOUNDARY("(\\b|\\s|$)");

}  // namespace

namespace chatterino {

class Nickname
{
public:
    Nickname(const QString &name, const QString &replace, const bool isRegex,
             const bool isCaseSensitive)
        : name_(name)
        , replace_(replace)
        , isRegex_(isRegex)
        , isCaseSensitive_(isCaseSensitive)
        , regex_(isRegex_
                     ? name
                     : REGEX_START_BOUNDARY + QRegularExpression::escape(name) +
                           REGEX_END_BOUNDARY,
                 QRegularExpression::UseUnicodePropertiesOption |
                     (isCaseSensitive_
                          ? QRegularExpression::NoPatternOption
                          : QRegularExpression::CaseInsensitiveOption))
    {
    }

    const QString &name() const
    {
        return this->name_;
    }
    const QString &replace() const
    {
        return this->replace_;
    }
    const bool &isRegex() const
    {
        return this->isRegex_;
    }
    const bool &isCaseSensitive() const
    {
        return this->isCaseSensitive_;
    }
    const QRegularExpression &regex() const
    {
        return this->regex_;
    }

private:
    QString name_;
    QString replace_;
    bool isRegex_;
    bool isCaseSensitive_;
    QRegularExpression regex_;
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::Nickname> {
    static rapidjson::Value get(const chatterino::Nickname &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "name", value.name(), a);
        chatterino::rj::set(ret, "replace", value.replace(), a);
        chatterino::rj::set(ret, "isRegex", value.isRegex(), a);
        chatterino::rj::set(ret, "isCaseSensitive", value.isCaseSensitive(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::Nickname> {
    static chatterino::Nickname get(const rapidjson::Value &value,
                                    bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return chatterino::Nickname(QString(), QString(), false, true);
        }

        QString _name;
        QString _replace;
        bool _isRegex;
        bool _isCaseSensitive;

        chatterino::rj::getSafe(value, "name", _name);
        chatterino::rj::getSafe(value, "replace", _replace);
        chatterino::rj::getSafe(value, "isRegex", _isRegex);
        chatterino::rj::getSafe(value, "isCaseSensitive", _isCaseSensitive);

        return chatterino::Nickname(_name, _replace, _isRegex,
                                    _isCaseSensitive);
    }
};

}  // namespace pajlada
