#pragma once

#include "common/Aliases.hpp"
#include "util/RapidjsonHelpers.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <pajlada/serialize.hpp>
#include <QRegularExpression>
#include <QString>

#include <memory>
#include <unordered_map>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class IgnorePhrase
{
public:
    IgnorePhrase(const QString &pattern, bool isRegex, bool isBlock,
                 const QString &replace, bool isCaseSensitive);

    bool operator==(const IgnorePhrase &other) const;

    const QString &getPattern() const;

    bool isRegex() const;

    bool isRegexValid() const;

    bool isMatch(const QString &subject) const;

    const QRegularExpression &getRegex() const;

    bool isBlock() const;

    const QString &getReplace() const;

    bool isCaseSensitive() const;

    Qt::CaseSensitivity caseSensitivity() const;

    const std::unordered_map<EmoteName, EmotePtr> &getEmotes() const;

    bool containsEmote() const;

    static IgnorePhrase createEmpty();

private:
    QString pattern_;
    bool isRegex_;
    QRegularExpression regex_;
    bool isBlock_;
    QString replace_;
    bool isCaseSensitive_;
    mutable std::unordered_map<EmoteName, EmotePtr> emotes_;
    mutable bool emotesChecked_{false};
};
}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::IgnorePhrase> {
    static rapidjson::Value get(const chatterino::IgnorePhrase &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "pattern", value.getPattern(), a);
        chatterino::rj::set(ret, "regex", value.isRegex(), a);
        chatterino::rj::set(ret, "isBlock", value.isBlock(), a);
        chatterino::rj::set(ret, "replaceWith", value.getReplace(), a);
        chatterino::rj::set(ret, "caseSensitive", value.isCaseSensitive(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::IgnorePhrase> {
    static chatterino::IgnorePhrase get(const rapidjson::Value &value,
                                        bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return chatterino::IgnorePhrase::createEmpty();
        }

        QString _pattern;
        bool _isRegex = false;
        bool _isBlock = false;
        QString _replace;
        bool _caseSens = true;

        chatterino::rj::getSafe(value, "pattern", _pattern);
        chatterino::rj::getSafe(value, "regex", _isRegex);
        chatterino::rj::getSafe(value, "isBlock", _isBlock);
        chatterino::rj::getSafe(value, "replaceWith", _replace);
        chatterino::rj::getSafe(value, "caseSensitive", _caseSens);

        return chatterino::IgnorePhrase(_pattern, _isRegex, _isBlock, _replace,
                                        _caseSens);
    }
};

}  // namespace pajlada
