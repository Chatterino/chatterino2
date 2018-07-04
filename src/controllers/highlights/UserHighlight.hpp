#pragma once

#include "common/SerializeCustom.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <QRegularExpression>
#include <QString>
#include <pajlada/settings/serialize.hpp>

#include <memory>

namespace chatterino {

class UserHighlight
{
    QString pattern_;
    bool alert_;
    bool sound_;
    bool isRegex_;
    QRegularExpression regex_;

public:
    bool operator==(const UserHighlight &other) const
    {
        return std::tie(this->pattern_, this->alert_, this->sound_, this->isRegex_) ==
               std::tie(other.pattern_, other.alert_, other.sound_, other.isRegex_);
    }

    UserHighlight(const QString &pattern, bool alert, bool sound, bool isRegex = false)
        : pattern_(pattern)
        , alert_(alert)
        , sound_(sound)
        , isRegex_(isRegex)
        , regex_(isRegex ? pattern : "", QRegularExpression::CaseInsensitiveOption |
                                             QRegularExpression::UseUnicodePropertiesOption)
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

    bool isValidRegex() const
    {
        return this->isRegex() && this->regex_.isValid();
    }

    bool isMatch(const QString &subject) const
    {
        if (this->isRegex()) {
            if (this->isValidRegex()) {
                return this->regex_.match(subject).hasMatch();
            }

            return false;
        }
        return subject.toLower() == this->pattern_.toLower();
    }
};

}  // namespace chatterino

namespace pajlada {
namespace Settings {

template <>
struct Serialize<chatterino::UserHighlight> {
    static rapidjson::Value get(const chatterino::UserHighlight &value,
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
struct Deserialize<chatterino::UserHighlight> {
    static chatterino::UserHighlight get(const rapidjson::Value &value)
    {
        QString pattern;
        bool alert = true;
        bool sound = false;
        bool isRegex = false;

        if (!value.IsObject()) {
            return chatterino::UserHighlight(pattern, alert, sound, isRegex);
        }

        chatterino::rj::getSafe(value, "pattern", pattern);
        chatterino::rj::getSafe(value, "alert", alert);
        chatterino::rj::getSafe(value, "sound", sound);
        chatterino::rj::getSafe(value, "regex", isRegex);

        return chatterino::UserHighlight(pattern, alert, sound, isRegex);
    }
};

}  // namespace Settings
}  // namespace pajlada
