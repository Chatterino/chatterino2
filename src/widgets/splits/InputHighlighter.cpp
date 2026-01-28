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

bool isIgnoredWord(TwitchChannel *twitch, const QString &word)
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

        QString chatter;
        // skip '@' to allow @chatter
        if (word.startsWith('@'))
        {
            chatter = word.sliced(1);
        }
        else
        {
            chatter = word;
        }
        if (twitch->accessChatters()->contains(chatter) ||
            (getSettings()->alwaysIncludeBroadcasterInUserCompletions &&
             chatter.toLower() == twitch->getName().toLower()))
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

    // TODO: Replace this with a link parser variant that doesn't return the parsed data
    auto link = linkparser::parse(word);
    return link.has_value();
}

}  // namespace

namespace chatterino {

InputHighlighter::InputHighlighter(SpellChecker &spellChecker, QObject *parent)
    : QSyntaxHighlighter(parent)
    , spellChecker(spellChecker)
    // FIXME: this also matches URLs - this probably needs to be some function like Firefox' mozEnglishWordUtils::FindNextWord
    , wordRegex(R"(\p{L}(?:\P{Z}+\p{L}+)*)",
                QRegularExpression::PatternOption::UseUnicodePropertiesOption)
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

void InputHighlighter::highlightBlock(const QString &text)
{
    if (!this->spellChecker.isLoaded())
    {
        return;
    }
    auto *channel = this->channel.lock().get();

    QStringView textView = text;

    // skip leading command trigger
    auto cmdTriggerLen = getApp()->getCommands()->commandTriggerLen(textView);
    textView = textView.sliced(cmdTriggerLen);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    auto tokenIt = this->tokenRegex.globalMatchView(textView);
#else
    auto tokenIt = this->tokenRegex.globalMatch(textView);
#endif

    // iterate over whitespace-delimited tokens
    while (tokenIt.hasNext())
    {
        auto tokenMatch = tokenIt.next();
        auto token = tokenMatch.captured();
        if (isIgnoredWord(channel, token))
        {
            continue;
        }

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        auto wordIt = this->wordRegex.globalMatchView(token);
#else
        auto wordIt = this->wordRegex.globalMatch(token);
#endif

        while (wordIt.hasNext())
        {
            auto wordMatch = wordIt.next();
            auto word = wordMatch.captured();

            if (!this->spellChecker.check(word))
            {
                this->setFormat(static_cast<int>(cmdTriggerLen +
                                                 tokenMatch.capturedStart() +
                                                 wordMatch.capturedStart()),
                                static_cast<int>(word.size()), this->spellFmt);
            }
        }
    }
}

}  // namespace chatterino
