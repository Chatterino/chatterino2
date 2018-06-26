#pragma once

#include "util/rapidjson-helpers.hpp"
#include "util/serialize-custom.hpp"

#include <QRegularExpression>
#include <QString>
#include <memory>
#include <pajlada/settings/serialize.hpp>

namespace chatterino {
namespace controllers {
namespace highlights {

class HighlightPhrase
{
    QString pattern;
    bool alert;
    bool sound;
    bool _isRegex;
    QRegularExpression regex;

public:
    bool operator==(const HighlightPhrase &other) const
    {
        return std::tie(this->pattern, this->sound, this->alert, this->_isRegex) ==
               std::tie(other.pattern, other.sound, other.alert, other._isRegex);
    }

    HighlightPhrase(const QString &_pattern, bool _alert, bool _sound, bool isRegex)
        : pattern(_pattern)
        , alert(_alert)
        , sound(_sound)
        , _isRegex(isRegex)
        , regex(_isRegex ? _pattern : "\\b" + QRegularExpression::escape(_pattern) + "\\b",
                QRegularExpression::CaseInsensitiveOption |
                    QRegularExpression::UseUnicodePropertiesOption)
    {
    }

    const QString &getPattern() const
    {
        return this->pattern;
    }
    bool getAlert() const
    {
        return this->alert;
    }
    bool getSound() const
    {
        return this->sound;
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

    //    const QRegularExpression &getRegex() const
    //    {
    //        return this->regex;
    //    }
};
}  // namespace highlights
}  // namespace controllers
}  // namespace chatterino

namespace pajlada {
namespace Settings {

template <>
struct Serialize<chatterino::controllers::highlights::HighlightPhrase> {
    static rapidjson::Value get(const chatterino::controllers::highlights::HighlightPhrase &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        AddMember(ret, "pattern", value.getPattern(), a);
        AddMember(ret, "alert", value.getAlert(), a);
        AddMember(ret, "sound", value.getSound(), a);
        AddMember(ret, "regex", value.isRegex(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::controllers::highlights::HighlightPhrase> {
    static chatterino::controllers::highlights::HighlightPhrase get(const rapidjson::Value &value)
    {
        if (!value.IsObject()) {
            return chatterino::controllers::highlights::HighlightPhrase(QString(), true, false,
                                                                        false);
        }

        QString _pattern;
        bool _alert = true;
        bool _sound = false;
        bool _isRegex = false;

        chatterino::rj::getSafe(value, "pattern", _pattern);
        chatterino::rj::getSafe(value, "alert", _alert);
        chatterino::rj::getSafe(value, "sound", _sound);
        chatterino::rj::getSafe(value, "regex", _isRegex);

        return chatterino::controllers::highlights::HighlightPhrase(_pattern, _alert, _sound,
                                                                    _isRegex);
    }
};

}  // namespace Settings
}  // namespace pajlada
