// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/splits/InputHighlighter.hpp"

#include "Application.hpp"
#include "common/Aliases.hpp"
#include "common/LinkParser.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/spellcheck/SpellChecker.hpp"
#include "messages/Emote.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"

#include <QTextCharFormat>
#include <QTextDocument>

namespace {

using namespace chatterino;

bool isEmote(TwitchChannel *twitch, const QString &word)
{
    EmoteName name{word};
    if (twitch)
    {
        if (twitch->bttvEmote(name) || twitch->ffzEmote(name) ||
            twitch->seventvEmote(name))
        {
            return true;
        }
        auto locals = twitch->localTwitchEmotes();
        if (locals->contains(name))
        {
            return true;
        }
    }
    if (getApp()->getBttvEmotes()->emote(name) ||
        getApp()->getFfzEmotes()->emote(name) ||
        getApp()->getSeventvEmotes()->globalEmote(name))
    {
        return true;
    }

    if (getApp()
            ->getAccounts()
            ->twitch.getCurrent()
            ->twitchEmote(name)
            .has_value())
    {
        return true;
    }

    return false;
}

bool isChatter(TwitchChannel *twitch, const QString &word)
{
    if (twitch)
    {
        if (twitch->accessChatters()->contains(word) ||
            (getSettings()->alwaysIncludeBroadcasterInUserCompletions &&
             word.compare(twitch->getName(), Qt::CaseInsensitive) == 0))
        {
            return true;
        }
    }
    return false;
}

bool isLink(const QString &token)
{
    // TODO: Replace this with a link parser variant that doesn't return the parsed data
    auto link = linkparser::parse(token);
    return link.has_value();
}

bool isIgnoredWord(TwitchChannel *twitch, const QString &word)
{
    return isEmote(twitch, word) || isChatter(twitch, word);
}

bool isIgnoredToken(TwitchChannel *twitch, const QString &token)
{
    return isEmote(twitch, token) || isLink(token);
}

}  // namespace

namespace chatterino {

namespace inputhighlight::detail {

// A word is a string of unicode letters. Words are seperated by whitespace
// (tokenRegex) or, inside a token, by punctuation characters (except '_')
QRegularExpression wordRegex()
{
    static QRegularExpression regex{
        R"((?<=^|(?!_)\p{P})\p{L}+(?=$|(?!_)\p{P}))",
        QRegularExpression::PatternOption::UseUnicodePropertiesOption,
    };
    return regex;
}

}  // namespace inputhighlight::detail

InputHighlighter::InputHighlighter(SpellChecker &spellChecker, QObject *parent)
    : QSyntaxHighlighter(parent)
    , spellChecker(spellChecker)
    , wordRegex(inputhighlight::detail::wordRegex())
    , tokenRegex(R"(\S+)")
{
    this->spellFmt.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    this->spellFmt.setUnderlineColor(Qt::red);
}
InputHighlighter::~InputHighlighter() = default;

void InputHighlighter::setChannel(const std::shared_ptr<Channel> &channel)
{
    auto twitch = std::dynamic_pointer_cast<TwitchChannel>(channel);
    this->channel = twitch;
    this->rehighlight();
}

std::vector<QString> InputHighlighter::getSpellCheckedWords(const QString &text)
{
    std::vector<QString> words;
    this->visitWords(text, [&](const QString &word, qsizetype /*start*/,
                               qsizetype /*count*/) {
        words.emplace_back(word);
    });
    return words;
}

void InputHighlighter::highlightBlock(const QString &text)
{
    if (!this->spellChecker.isLoaded())
    {
        return;
    }
    this->visitWords(
        text, [&](const QString &word, qsizetype start, qsizetype count) {
            if (!this->spellChecker.check(word))
            {
                this->setFormat(static_cast<int>(start),
                                static_cast<int>(count), this->spellFmt);
            }
        });
}

void InputHighlighter::visitWords(
    const QString &text,
    std::invocable<const QString &, qsizetype, qsizetype> auto &&cb)
{
    auto *channel = this->channel.lock().get();

    QStringView textView = text;

    // skip leading command trigger
    auto cmdTriggerLen = getApp()->getCommands()->commandTriggerLen(textView);
    textView = textView.sliced(cmdTriggerLen);

    auto tokenIt = this->tokenRegex.globalMatchView(textView);

    // iterate over whitespace-delimited tokens
    while (tokenIt.hasNext())
    {
        auto tokenMatch = tokenIt.next();
        auto token = tokenMatch.captured();
        if (isIgnoredToken(channel, token))
        {
            continue;
        }

        auto wordIt = this->wordRegex.globalMatchView(token);

        while (wordIt.hasNext())
        {
            auto wordMatch = wordIt.next();
            auto word = wordMatch.captured();

            if (!isIgnoredWord(channel, word))
            {
                cb(word,
                   static_cast<int>(cmdTriggerLen + tokenMatch.capturedStart() +
                                    wordMatch.capturedStart()),
                   static_cast<int>(word.size()));
            }
        }
    }
}

}  // namespace chatterino
