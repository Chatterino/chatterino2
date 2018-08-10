#pragma once

#include "util/RapidJsonSerializeQString.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <QRegularExpression>
#include <QString>
#include <pajlada/settings/serialize.hpp>

#include <memory>

namespace chatterino {

class IgnorePhrase
{
public:
    bool operator==(const IgnorePhrase &other) const
    {
        return std::tie(this->pattern_, this->isRegex_) ==
               std::tie(other.pattern_, other.isRegex_);
    }

    IgnorePhrase(const QString &pattern, bool isRegex)
        : pattern_(pattern)
        , isRegex_(isRegex)
        , regex_(isRegex_ ? pattern
                          : "\\b" + QRegularExpression::escape(pattern) + "\\b",
                 QRegularExpression::CaseInsensitiveOption |
                     QRegularExpression::UseUnicodePropertiesOption)
    {
    }

    const QString &getPattern() const
    {
        return this->pattern_;
    }
    bool isRegex() const
    {
        return this->isRegex_;
    }

    bool isValid() const
    {
        return !this->pattern_.isEmpty() && this->regex_.isValid();
    }

    bool isMatch(const QString &subject) const
    {
        return this->isValid() && this->regex_.match(subject).hasMatch();
    }

private:
    QString pattern_;
    bool isRegex_;
    QRegularExpression regex_;
};
}  // namespace chatterino

namespace pajlada {
namespace Settings {

template <>
struct Serialize<chatterino::IgnorePhrase> {
    static rapidjson::Value get(const chatterino::IgnorePhrase &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        AddMember(ret, "pattern", value.getPattern(), a);
        AddMember(ret, "regex", value.isRegex(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::IgnorePhrase> {
    static chatterino::IgnorePhrase get(const rapidjson::Value &value)
    {
        if (!value.IsObject()) {
            return chatterino::IgnorePhrase(QString(), false);
        }

        QString _pattern;
        bool _isRegex = false;

        chatterino::rj::getSafe(value, "pattern", _pattern);
        chatterino::rj::getSafe(value, "regex", _isRegex);

        return chatterino::IgnorePhrase(_pattern, _isRegex);
    }
};

}  // namespace Settings
}  // namespace pajlada
