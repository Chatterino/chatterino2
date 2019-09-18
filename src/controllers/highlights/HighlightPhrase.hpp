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
        return std::tie(this->pattern_, this->hasSound_, this->hasAlert_,
                        this->isRegex_, this->isCaseSensitive_) ==
               std::tie(other.pattern_, other.hasSound_, other.hasAlert_,
                        other.isRegex_, other.isCaseSensitive_);
    }

    HighlightPhrase(const QString &pattern, bool hasAlert, bool hasSound,
                    bool isRegex, bool isCaseSensitive)
        : pattern_(pattern)
        , hasAlert_(hasAlert)
        , hasSound_(hasSound)
        , isRegex_(isRegex)
        , isCaseSensitive_(isCaseSensitive)
        , regex_(
              isRegex_ ? pattern
                       : "\\b" + QRegularExpression::escape(pattern) + "\\b",
              QRegularExpression::UseUnicodePropertiesOption |
                  (isCaseSensitive_ ? QRegularExpression::NoPatternOption
                                  : QRegularExpression::CaseInsensitiveOption))
    {
    }

    const QString &getPattern() const
    {
        return this->pattern_;
    }
    bool hasAlert() const
    {
        return this->hasAlert_;
    }
    bool hasSound() const
    {
        return this->hasSound_;
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
        return this->isCaseSensitive_;
    }
    
private:
    QString pattern_;
    bool hasAlert_;
    bool hasSound_;
    bool isRegex_;
    bool isCaseSensitive_;
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
        chatterino::rj::set(ret, "alert", value.hasAlert(), a);
        chatterino::rj::set(ret, "sound", value.hasSound(), a);
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
        bool _hasAlert = true;
        bool _hasSound = false;
        bool _isRegex = false;
        bool _isCaseSensitive = false;

        chatterino::rj::getSafe(value, "pattern", _pattern);
        chatterino::rj::getSafe(value, "alert", _hasAlert);
        chatterino::rj::getSafe(value, "sound", _hasSound);
        chatterino::rj::getSafe(value, "regex", _isRegex);
        chatterino::rj::getSafe(value, "case", _isCaseSensitive);

            return chatterino::HighlightPhrase(_pattern, _hasAlert, _hasSound,
                                               _isRegex, _isCaseSensitive);
    }
};

}  // namespace pajlada
