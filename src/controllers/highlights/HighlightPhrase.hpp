#pragma once

#include "util/RapidJsonSerializeQString.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <QRegularExpression>
#include <QString>
#include <pajlada/serialize.hpp>

namespace chatterino {

class HighlightPhrase
{
public:
    bool operator==(const HighlightPhrase &other) const
    {
        return std::tie(this->pattern_, this->sound_, this->alert_,
                        this->isRegex_, this->caseSensitive_) ==
               std::tie(other.pattern_, other.sound_, other.alert_,
                        other.isRegex_, other.caseSensitive_);
    }

    HighlightPhrase(const QString &pattern, bool alert, bool sound,
                    bool isRegex, bool caseSensitive)
        : pattern_(pattern)
        , alert_(alert)
        , sound_(sound)
        , isRegex_(isRegex)
        , caseSensitive_(caseSensitive)
        , regex_(
              isRegex_ ? pattern
                       : "\\b" + QRegularExpression::escape(pattern) + "\\b",
              QRegularExpression::UseUnicodePropertiesOption |
                  (caseSensitive_ ? QRegularExpression::NoPatternOption
                                  : QRegularExpression::CaseInsensitiveOption))
    {
    }

    const QString &getPattern() const
    {
        return this->pattern_;
    }
    bool getAlert() const
    {
        return this->alert_;
    }
    bool getSound() const
    {
        return this->sound_;
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

    bool isCaseSensitive() const
    {
        return this->caseSensitive_;
    }

private:
    QString pattern_;
    bool alert_;
    bool sound_;
    bool isRegex_;
    bool caseSensitive_;
    QRegularExpression regex_;
};
}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::HighlightPhrase> {
    static rapidjson::Value get(const chatterino::HighlightPhrase &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "pattern", value.getPattern(), a);
        chatterino::rj::set(ret, "alert", value.getAlert(), a);
        chatterino::rj::set(ret, "sound", value.getSound(), a);
        chatterino::rj::set(ret, "regex", value.isRegex(), a);
        chatterino::rj::set(ret, "case", value.isCaseSensitive(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::HighlightPhrase> {
    static chatterino::HighlightPhrase get(const rapidjson::Value &value)
    {
        if (!value.IsObject())
        {
            return chatterino::HighlightPhrase(QString(), true, false, false,
                                               false);
        }

        QString _pattern;
        bool _alert = true;
        bool _sound = false;
        bool _isRegex = false;
        bool _caseSensitive = false;

        chatterino::rj::getSafe(value, "pattern", _pattern);
        chatterino::rj::getSafe(value, "alert", _alert);
        chatterino::rj::getSafe(value, "sound", _sound);
        chatterino::rj::getSafe(value, "regex", _isRegex);
        chatterino::rj::getSafe(value, "case", _caseSensitive);

            return chatterino::HighlightPhrase(_pattern, _alert, _sound,
                                               _isRegex, _caseSensitive);
    }
};

}  // namespace pajlada
