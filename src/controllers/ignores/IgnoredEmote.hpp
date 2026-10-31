// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "util/RapidjsonHelpers.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <pajlada/serialize.hpp>
#include <QRegularExpression>
#include <QString>

#include <utility>

namespace chatterino {

struct IgnoredEmote {
    explicit IgnoredEmote(QString pattern = {}, bool regex = false)
        : pattern(std::move(pattern))
        , regex(regex)
        , compiledRegex(this->pattern,
                        QRegularExpression::UseUnicodePropertiesOption)
    {
    }

    bool operator==(const IgnoredEmote &other) const
    {
        return this->pattern == other.pattern && this->regex == other.regex;
    }

    bool isMatch(const QString &name) const
    {
        if (this->pattern.isEmpty())
        {
            return false;
        }
        if (this->regex)
        {
            return this->compiledRegex.isValid() &&
                   this->compiledRegex.match(name).hasMatch();
        }
        return name == this->pattern;
    }

    QString pattern;
    bool regex;

private:
    QRegularExpression compiledRegex;
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::IgnoredEmote> {
    static rapidjson::Value get(const chatterino::IgnoredEmote &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value result(rapidjson::kObjectType);
        chatterino::rj::set(result, "pattern", value.pattern, a);
        chatterino::rj::set(result, "regex", value.regex, a);
        return result;
    }
};

template <>
struct Deserialize<chatterino::IgnoredEmote> {
    static chatterino::IgnoredEmote get(const rapidjson::Value &value,
                                        bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return chatterino::IgnoredEmote{};
        }

        QString pattern;
        bool regex = false;
        chatterino::rj::getSafe(value, "pattern", pattern);
        chatterino::rj::getSafe(value, "regex", regex);
        return chatterino::IgnoredEmote{std::move(pattern), regex};
    }
};

}  // namespace pajlada
