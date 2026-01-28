// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/splits/InputHighlighter.hpp"

#include "Application.hpp"
#include "common/Aliases.hpp"
#include "common/LinkParser.hpp"
#include "common/QLogging.hpp"
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
    qCDebug(chatterinoSpellcheck) << "isIgnoredWord" << word;
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

    QRegularExpression outerRegex(R"(\S+)");
    // maybe change to  R"(\p{L}+)" to allow for more/any seperators in words (would fix #6762)
    QRegularExpression innerRegex = this->wordRegex;

    QStringView textView = text;

    // skip leading command trigger
    auto cmdTriggerLen = getApp()->getCommands()->commandTriggerLen(textView);
    textView = textView.sliced(cmdTriggerLen);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    auto outerIt = outerRegex.globalMatchView(textView);
#else
    auto outerIt = outerRegex.globalMatch(textView);
#endif

    // iterate over strings of any non-whitespace characters
    while (outerIt.hasNext())
    {
        auto outerMatch = outerIt.next();
        auto outerText = outerMatch.captured();
        if (isIgnoredWord(channel, outerText))
        {
            continue;
        }

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        auto innerIt = innerRegex.globalMatchView(outerText);
#else
        auto innerIt = innerRegex.globalMatch(outerText);
#endif

        // iterate over words that match the word regex
        while (innerIt.hasNext())
        {
            auto innerMatch = innerIt.next();
            auto innerText = innerMatch.captured();

            qCDebug(chatterinoSpellcheck) << "check" << innerText;
            if (!this->spellChecker.check(innerText))
            {
                this->setFormat(static_cast<int>(cmdTriggerLen +
                                                 outerMatch.capturedStart() +
                                                 innerMatch.capturedStart()),
                                static_cast<int>(innerText.size()),
                                this->spellFmt);
            }
        }
    }
}

}  // namespace chatterino
