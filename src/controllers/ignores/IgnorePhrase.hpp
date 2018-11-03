#pragma once

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "singletons/Settings.hpp"

#include "util/RapidJsonSerializeQString.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <QRegularExpression>
#include <QString>
#include <pajlada/serialize.hpp>

#include <memory>

namespace chatterino {

class IgnorePhrase
{
public:
    bool operator==(const IgnorePhrase &other) const
    {
        return std::tie(this->pattern_, this->isRegex_, this->isBlock_,
                        this->replace_, this->isCaseSensitive_) ==
               std::tie(other.pattern_, other.isRegex_, other.isBlock_,
                        other.replace_, other.isCaseSensitive_);
    }

    IgnorePhrase(const QString &pattern, bool isRegex, bool isBlock,
                 const QString &replace, bool isCaseSensitive)
        : pattern_(pattern)
        , isRegex_(isRegex)
        , regex_(pattern)
        , isBlock_(isBlock)
        , replace_(replace)
        , isCaseSensitive_(isCaseSensitive)
    {
        if (this->isCaseSensitive_)
        {
            regex_.setPatternOptions(
                QRegularExpression::UseUnicodePropertiesOption);
        }
        else
        {
            regex_.setPatternOptions(
                QRegularExpression::CaseInsensitiveOption |
                QRegularExpression::UseUnicodePropertiesOption);
        }
    }

    const QString &getPattern() const
    {
        return this->pattern_;
    }

    bool isRegex() const
    {
        return this->isRegex_;
    }

    bool isRegexValid() const
    {
        return this->regex_.isValid();
    }

    bool isMatch(const QString &subject) const
    {
        return !this->pattern_.isEmpty() &&
               (this->isRegex() ? (this->regex_.isValid() &&
                                   this->regex_.match(subject).hasMatch())
                                : subject.contains(this->pattern_,
                                                   this->caseSensitivity()));
    }

    const QRegularExpression &getRegex() const
    {
        return this->regex_;
    }

    bool isBlock() const
    {
        return this->isBlock_;
    }

    const QString &getReplace() const
    {
        return this->replace_;
    }

    bool isCaseSensitive() const
    {
        return this->isCaseSensitive_;
    }

    Qt::CaseSensitivity caseSensitivity() const
    {
        return this->isCaseSensitive_ ? Qt::CaseSensitive : Qt::CaseInsensitive;
    }

    const std::unordered_map<EmoteName, EmotePtr> &getEmotes() const
    {
        return this->emotes_;
    }

    bool containsEmote() const
    {
        if (!this->emotesChecked_)
        {
            const auto &accvec =
                getApp()->accounts->twitch.accounts.getVector();
            for (const auto &acc : accvec)
            {
                const auto &accemotes = *acc->accessEmotes();
                for (const auto &emote : accemotes.emotes)
                {
                    if (this->replace_.contains(emote.first.string,
                                                Qt::CaseSensitive))
                    {
                        this->emotes_.emplace(emote.first, emote.second);
                    }
                }
            }
            this->emotesChecked_ = true;
        }
        return !this->emotes_.empty();
    }

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
    static chatterino::IgnorePhrase get(const rapidjson::Value &value)
    {
        if (!value.IsObject())
        {
            return chatterino::IgnorePhrase(
                QString(), false, false,
                ::chatterino::getSettings()->ignoredPhraseReplace.getValue(),
                true);
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
