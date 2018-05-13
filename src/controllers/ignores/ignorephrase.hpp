#pragma once

#include "util/rapidjson-helpers.hpp"
#include "util/serialize-custom.hpp"

#include <QRegularExpression>
#include <QString>
#include <pajlada/settings/serialize.hpp>

#include <memory>

namespace chatterino {
namespace controllers {
namespace ignores {

class IgnorePhrase
{
    QString pattern;
    bool _isRegex;
    QRegularExpression regex;

public:
    bool operator==(const IgnorePhrase &other) const
    {
        return std::tie(this->pattern, this->_isRegex) == std::tie(other.pattern, other._isRegex);
    }

    IgnorePhrase(const QString &_pattern, bool isRegex)
        : pattern(_pattern)
        , _isRegex(isRegex)
        , regex(_isRegex ? _pattern : "\\b" + QRegularExpression::escape(_pattern) + "\\b",
                QRegularExpression::CaseInsensitiveOption)
    {
    }

    const QString &getPattern() const
    {
        return this->pattern;
    }
    bool isRegex() const
    {
        return this->_isRegex;
    }

    bool isValid() const
    {
        return !this->pattern.isEmpty() && this->regex.isValid();
    }

    bool isMatch(const QString &subject) const
    {
        return this->isValid() && this->regex.match(subject).hasMatch();
    }
};
}  // namespace ignores
}  // namespace controllers
}  // namespace chatterino

namespace pajlada {
namespace Settings {

template <>
struct Serialize<chatterino::controllers::ignores::IgnorePhrase> {
    static rapidjson::Value get(const chatterino::controllers::ignores::IgnorePhrase &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        AddMember(ret, "pattern", value.getPattern(), a);
        AddMember(ret, "regex", value.isRegex(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::controllers::ignores::IgnorePhrase> {
    static chatterino::controllers::ignores::IgnorePhrase get(const rapidjson::Value &value)
    {
        if (!value.IsObject()) {
            return chatterino::controllers::ignores::IgnorePhrase(QString(), false);
        }

        QString _pattern;
        bool _isRegex = false;

        chatterino::rj::getSafe(value, "pattern", _pattern);
        chatterino::rj::getSafe(value, "regex", _isRegex);

        return chatterino::controllers::ignores::IgnorePhrase(_pattern, _isRegex);
    }
};

}  // namespace Settings
}  // namespace pajlada
