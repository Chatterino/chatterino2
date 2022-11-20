#pragma once

#include "util/RapidjsonHelpers.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <pajlada/serialize.hpp>
#include <QRegularExpression>
#include <QString>

#include <memory>

namespace chatterino {

class HighlightBlacklistUser
{
public:
    bool operator==(const HighlightBlacklistUser &other) const
    {
        return std::tie(this->pattern_, this->isRegex_) ==
               std::tie(other.pattern_, other.isRegex_);
    }

    HighlightBlacklistUser(const QString &pattern, bool isRegex = false)
        : pattern_(pattern)
        , isRegex_(isRegex)
        , regex_(isRegex ? pattern : "",
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

    bool isValidRegex() const
    {
        return this->isRegex() && this->regex_.isValid();
    }

    bool isMatch(const QString &subject) const
    {
        if (this->isRegex())
        {
            if (this->isValidRegex())
            {
                return this->regex_.match(subject).hasMatch();
            }

            return false;
        }

        return subject.toLower() == this->pattern_.toLower();
    }

private:
    QString pattern_;
    bool isRegex_;
    QRegularExpression regex_;
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::HighlightBlacklistUser> {
    static rapidjson::Value get(const chatterino::HighlightBlacklistUser &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "pattern", value.getPattern(), a);
        chatterino::rj::set(ret, "regex", value.isRegex(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::HighlightBlacklistUser> {
    static chatterino::HighlightBlacklistUser get(const rapidjson::Value &value,
                                                  bool *error = nullptr)
    {
        QString pattern;
        bool isRegex = false;

        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return chatterino::HighlightBlacklistUser(pattern, isRegex);
        }

        chatterino::rj::getSafe(value, "pattern", pattern);
        chatterino::rj::getSafe(value, "regex", isRegex);

        return chatterino::HighlightBlacklistUser(pattern, isRegex);
    }
};

}  // namespace pajlada
