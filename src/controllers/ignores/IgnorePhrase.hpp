#pragma once

#include "common/SerializeCustom.hpp"
#include "singletons/Settings.hpp"
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
        return std::tie(this->pattern_, this->isRegex_, this->isReplace_, this->replace_) ==
               std::tie(other.pattern_, other.isRegex_, other.isReplace_, other.replace_);
    }

    IgnorePhrase(const QString &pattern, bool isRegex, bool isReplace, const QString &replace)
        : pattern_(pattern)
        , isRegex_(isRegex)
        , regex_(isRegex_ ? pattern : "\\b" + QRegularExpression::escape(pattern) + "\\b",
                 QRegularExpression::CaseInsensitiveOption |
                     QRegularExpression::UseUnicodePropertiesOption)
        , isReplace_(isReplace)
        , replace_(replace)
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

    bool isReplace() const
    {
        return this->isReplace_;
    }

    const QString &getReplace() const
    {
        return this->replace_;
    }

private:
    QString pattern_;
    bool isRegex_;
    QRegularExpression regex_;
    bool isReplace_;
    QString replace_;
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
        AddMember(ret, "onlyWord", value.isReplace(), a);
        AddMember(ret, "replace", value.getReplace(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::IgnorePhrase> {
    static chatterino::IgnorePhrase get(const rapidjson::Value &value)
    {
        if (!value.IsObject()) {
            return chatterino::IgnorePhrase(
                QString(), false, false,
                ::chatterino::getSettings()->ignoredPhraseReplace.getValue());
        }

        QString _pattern;
        bool _isRegex = false;
        bool _isReplace = false;
        QString _replace;

        chatterino::rj::getSafe(value, "pattern", _pattern);
        chatterino::rj::getSafe(value, "regex", _isRegex);
        chatterino::rj::getSafe(value, "onlyWord", _isReplace);
        chatterino::rj::getSafe(value, "replace", _replace);

        return chatterino::IgnorePhrase(_pattern, _isRegex, _isReplace, _replace);
    }
};

}  // namespace Settings
}  // namespace pajlada
