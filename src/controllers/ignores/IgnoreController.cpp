#include "controllers/ignores/IgnoreController.hpp"

#include "Application.hpp"
#include "common/Literals.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchIrc.hpp"
#include "singletons/Settings.hpp"

namespace {

using namespace chatterino::literals;

/**
  * Computes (only) the replacement of @a match in @a source.
  * The parts before and after the match in @a source are ignored.
  *
  * Occurrences of \b{\\1}, \b{\\2}, ..., in @a replacement are replaced
  * with the string captured by the corresponding capturing group.
  * This function should only be used if the regex contains capturing groups.
  * 
  * Since Qt doesn't provide a way of replacing a single match with some replacement
  * while supporting both capturing groups and lookahead/-behind in the regex,
  * this is included here. It's essentially the implementation of 
  * QString::replace(const QRegularExpression &, const QString &).
  * @see https://github.com/qt/qtbase/blob/97bb0ecfe628b5bb78e798563212adf02129c6f6/src/corelib/text/qstring.cpp#L4594-L4703
  */
QString makeRegexReplacement(QStringView source,
                             const QRegularExpression &regex,
                             const QRegularExpressionMatch &match,
                             const QString &replacement)
{
    using SizeType = QString::size_type;
    struct QStringCapture {
        SizeType pos;
        SizeType len;
        int captureNumber;
    };

    qsizetype numCaptures = regex.captureCount();

    // 1. build the backreferences list, holding where the backreferences
    //    are in the replacement string
    QVarLengthArray<QStringCapture> backReferences;

    SizeType replacementLength = replacement.size();
    for (SizeType i = 0; i < replacementLength - 1; i++)
    {
        if (replacement[i] != u'\\')
        {
            continue;
        }

        int no = replacement[i + 1].digitValue();
        if (no <= 0 || no > numCaptures)
        {
            continue;
        }

        QStringCapture backReference{.pos = i, .len = 2};

        if (i < replacementLength - 2)
        {
            int secondDigit = replacement[i + 2].digitValue();
            if (secondDigit != -1 && ((no * 10) + secondDigit) <= numCaptures)
            {
                no = (no * 10) + secondDigit;
                ++backReference.len;
            }
        }

        backReference.captureNumber = no;
        backReferences.append(backReference);
    }

    // 2. iterate on the matches.
    //    For every match, copy the replacement string in chunks
    //    with the proper replacements for the backreferences

    // length of the new string, with all the replacements
    SizeType newLength = 0;
    QVarLengthArray<QStringView> chunks;
    QStringView replacementView{replacement};

    // Initially: empty, as we only care about the replacement
    SizeType len = 0;
    SizeType lastEnd = 0;
    for (const QStringCapture &backReference : std::as_const(backReferences))
    {
        // part of "replacement" before the backreference
        len = backReference.pos - lastEnd;
        if (len > 0)
        {
            chunks << replacementView.mid(lastEnd, len);
            newLength += len;
        }

        // backreference itself
        len = match.capturedLength(backReference.captureNumber);
        if (len > 0)
        {
            chunks << source.mid(
                match.capturedStart(backReference.captureNumber), len);
            newLength += len;
        }

        lastEnd = backReference.pos + backReference.len;
    }

    // add the last part of the replacement string
    len = replacementView.size() - lastEnd;
    if (len > 0)
    {
        chunks << replacementView.mid(lastEnd, len);
        newLength += len;
    }

    // 3. assemble the chunks together
    QString dst;
    dst.reserve(newLength);
    for (const QStringView &chunk : std::as_const(chunks))
    {
        dst += chunk;
    }
    return dst;
}

}  // namespace

namespace chatterino {

bool isIgnoredMessage(IgnoredMessageParameters &&params)
{
    if (!params.message.isEmpty())
    {
        // TODO(pajlada): Do we need to check if the phrase is valid first?
        auto phrases = getSettings()->ignoredMessages.readOnly();
        for (const auto &phrase : *phrases)
        {
            if (phrase.isBlock() && phrase.isMatch(params.message))
            {
                qCDebug(chatterinoMessage)
                    << "Blocking message because it contains ignored phrase"
                    << phrase.getPattern();
                return true;
            }
        }
    }

    if (!params.twitchUserID.isEmpty() &&
        getSettings()->enableTwitchBlockedUsers)
    {
        auto sourceUserID = params.twitchUserID;

        bool isBlocked = getApp()
                             ->getAccounts()
                             ->twitch.getCurrent()
                             ->blockedUserIds()
                             .contains(sourceUserID);
        if (isBlocked)
        {
            switch (static_cast<ShowIgnoredUsersMessages>(
                getSettings()->showBlockedUsersMessages.getValue()))
            {
                case ShowIgnoredUsersMessages::IfModerator:
                    if (params.isMod || params.isBroadcaster)
                    {
                        return false;
                    }
                    break;
                case ShowIgnoredUsersMessages::IfBroadcaster:
                    if (params.isBroadcaster)
                    {
                        return false;
                    }
                    break;
                case ShowIgnoredUsersMessages::Never:
                    break;
            }

            return true;
        }
    }

    return false;
}

void processIgnorePhrases(const std::vector<IgnorePhrase> &phrases,
                          QString &content,
                          std::vector<TwitchEmoteOccurrence> &twitchEmotes)
{
    using SizeType = QString::size_type;

    auto removeEmotesInRange = [&twitchEmotes](SizeType pos, SizeType len) {
        // all emotes outside the range come before `it`
        // all emotes in the range start at `it`
        auto it = std::partition(
            twitchEmotes.begin(), twitchEmotes.end(),
            [pos, len](const auto &item) {
                // returns true for emotes outside the range
                return !((item.start >= pos) && item.start < (pos + len));
            });
        std::vector<TwitchEmoteOccurrence> emotesInRange(it,
                                                         twitchEmotes.end());
        twitchEmotes.erase(it, twitchEmotes.end());
        return emotesInRange;
    };

    auto shiftIndicesAfter = [&twitchEmotes](int pos, int by) {
        for (auto &item : twitchEmotes)
        {
            auto &index = item.start;
            if (index >= pos)
            {
                index += by;
                item.end += by;
            }
        }
    };

    auto addReplEmotes = [&twitchEmotes](const IgnorePhrase &phrase,
                                         const auto &midrepl,
                                         SizeType startIndex) {
        if (!phrase.containsEmote())
        {
            return;
        }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        auto words = midrepl.tokenize(u' ');
#else
        auto words = midrepl.split(' ');
#endif
        SizeType pos = 0;
        for (const auto &word : words)
        {
            for (const auto &emote : phrase.getEmotes())
            {
                if (word == emote.first.string)
                {
                    if (emote.second == nullptr)
                    {
                        qCDebug(chatterinoTwitch)
                            << "emote null" << emote.first.string;
                    }
                    twitchEmotes.push_back(TwitchEmoteOccurrence{
                        static_cast<int>(startIndex + pos),
                        static_cast<int>(startIndex + pos +
                                         emote.first.string.length()),
                        emote.second,
                        emote.first,
                    });
                }
            }
            pos += word.length() + 1;
        }
    };

    auto replaceMessageAt = [&](const IgnorePhrase &phrase, SizeType from,
                                SizeType length, const QString &replacement) {
        auto removedEmotes = removeEmotesInRange(from, length);
        content.replace(from, length, replacement);
        auto wordStart = from;
        while (wordStart > 0)
        {
            if (content[wordStart - 1] == ' ')
            {
                break;
            }
            --wordStart;
        }
        auto wordEnd = from + replacement.length();
        while (wordEnd < content.length())
        {
            if (content[wordEnd] == ' ')
            {
                break;
            }
            ++wordEnd;
        }

        shiftIndicesAfter(static_cast<int>(from + length),
                          static_cast<int>(replacement.length() - length));

        auto midExtendedRef =
            QStringView{content}.mid(wordStart, wordEnd - wordStart);

        for (auto &emote : removedEmotes)
        {
            if (emote.ptr == nullptr)
            {
                qCDebug(chatterinoTwitch)
                    << "Invalid emote occurrence" << emote.name.string;
                continue;
            }
            QRegularExpression emoteregex(
                "\\b" + emote.name.string + "\\b",
                QRegularExpression::UseUnicodePropertiesOption);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
            auto match = emoteregex.matchView(midExtendedRef);
#else
            auto match = emoteregex.match(midExtendedRef);
#endif
            if (match.hasMatch())
            {
                emote.start = static_cast<int>(from + match.capturedStart());
                emote.end = static_cast<int>(from + match.capturedEnd());
                twitchEmotes.push_back(std::move(emote));
            }
        }

        addReplEmotes(phrase, midExtendedRef, wordStart);
    };

    for (const auto &phrase : phrases)
    {
        if (phrase.isBlock())
        {
            continue;
        }
        const auto &pattern = phrase.getPattern();
        if (pattern.isEmpty())
        {
            continue;
        }
        if (phrase.isRegex())
        {
            const auto &regex = phrase.getRegex();
            if (!regex.isValid())
            {
                continue;
            }

            QRegularExpressionMatch match;
            size_t iterations = 0;
            SizeType from = 0;
            while ((from = content.indexOf(regex, from, &match)) != -1)
            {
                auto replacement = phrase.getReplace();
                if (regex.captureCount() > 0)
                {
                    replacement = makeRegexReplacement(content, regex, match,
                                                       replacement);
                }

                replaceMessageAt(phrase, from, match.capturedLength(),
                                 replacement);
                from += phrase.getReplace().length();
                iterations++;
                if (iterations >= 128)
                {
                    content = u"Too many replacements - check your ignores!"_s;
                    return;
                }
            }

            continue;
        }

        SizeType from = 0;
        while ((from = content.indexOf(pattern, from,
                                       phrase.caseSensitivity())) != -1)
        {
            replaceMessageAt(phrase, from, pattern.length(),
                             phrase.getReplace());
            from += phrase.getReplace().length();
        }
    }
}

}  // namespace chatterino
