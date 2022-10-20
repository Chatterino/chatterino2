#include "providers/twitch/TwitchMessageBuilder.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/ignores/IgnoreController.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "messages/Message.hpp"
#include "providers/chatterino/ChatterinoBadges.hpp"
#include "providers/ffz/FfzBadges.hpp"
#include "providers/seventv/SeventvBadges.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Helpers.hpp"
#include "util/IrcHelpers.hpp"
#include "util/Qt.hpp"
#include "widgets/Window.hpp"

#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QMediaPlayer>
#include <QStringRef>
#include <boost/variant.hpp>
#include "common/QLogging.hpp"

namespace {

const QString regexHelpString("(\\w+)[.,!?;:]*?$");

// matches a mention with punctuation at the end, like "@username," or "@username!!!" where capture group would return "username"
const QRegularExpression mentionRegex("^@" + regexHelpString);

// if findAllUsernames setting is enabled, matches strings like in the examples above, but without @ symbol at the beginning
const QRegularExpression allUsernamesMentionRegex("^" + regexHelpString);

const QSet<QString> zeroWidthEmotes{
    "SoSnowy",  "IceCold",   "SantaHat", "TopHat",
    "ReinDeer", "CandyCane", "cvMask",   "cvHazmat",
};

}  // namespace

namespace chatterino {

namespace {

    QString stylizeUsername(const QString &username, const Message &message)
    {
        auto app = getApp();

        const QString &localizedName = message.localizedName;
        bool hasLocalizedName = !localizedName.isEmpty();

        // The full string that will be rendered in the chat widget
        QString usernameText;

        switch (getSettings()->usernameDisplayMode.getValue())
        {
            case UsernameDisplayMode::Username: {
                usernameText = username;
            }
            break;

            case UsernameDisplayMode::LocalizedName: {
                if (hasLocalizedName)
                {
                    usernameText = localizedName;
                }
                else
                {
                    usernameText = username;
                }
            }
            break;

            default:
            case UsernameDisplayMode::UsernameAndLocalizedName: {
                if (hasLocalizedName)
                {
                    usernameText = username + "(" + localizedName + ")";
                }
                else
                {
                    usernameText = username;
                }
            }
            break;
        }

        auto nicknames = getCSettings().nicknames.readOnly();

        for (const auto &nickname : *nicknames)
        {
            if (nickname.match(usernameText))
            {
                break;
            }
        }

        return usernameText;
    }

}  // namespace

TwitchMessageBuilder::TwitchMessageBuilder(
    Channel *_channel, const Communi::IrcPrivateMessage *_ircMessage,
    const MessageParseArgs &_args)
    : SharedMessageBuilder(_channel, _ircMessage, _args)
    , twitchChannel(dynamic_cast<TwitchChannel *>(_channel))
{
}

TwitchMessageBuilder::TwitchMessageBuilder(
    Channel *_channel, const Communi::IrcMessage *_ircMessage,
    const MessageParseArgs &_args, QString content, bool isAction)
    : SharedMessageBuilder(_channel, _ircMessage, _args, content, isAction)
    , twitchChannel(dynamic_cast<TwitchChannel *>(_channel))
{
}

bool TwitchMessageBuilder::isIgnored() const
{
    return isIgnoredMessage({
        /*.message = */ this->originalMessage_,
        /*.twitchUserID = */ this->tags.value("user-id").toString(),
        /*.isMod = */ this->channel->isMod(),
        /*.isBroadcaster = */ this->channel->isBroadcaster(),
    });
}

void TwitchMessageBuilder::triggerHighlights()
{
    if (this->historicalMessage_)
    {
        // Do nothing. Highlights should not be triggered on historical messages.
        return;
    }

    SharedMessageBuilder::triggerHighlights();
}

MessagePtr TwitchMessageBuilder::build()
{
    // PARSE
    this->userId_ = this->ircMessage->tag("user-id").toString();

    this->parse();

    if (this->userName == this->channel->getName())
    {
        this->senderIsBroadcaster = true;
    }

    this->message().channelName = this->channel->getName();

    this->parseMessageID();

    this->parseRoomID();

    // If it is a reward it has to be appended first
    if (this->args.channelPointRewardId != "")
    {
        const auto &reward = this->twitchChannel->channelPointReward(
            this->args.channelPointRewardId);
        if (reward)
        {
            this->appendChannelPointRewardMessage(
                reward.get(), this, this->channel->isMod(),
                this->channel->isBroadcaster());
        }
    }

    this->appendChannelName();

    if (this->tags.contains("rm-deleted"))
    {
        this->message().flags.set(MessageFlag::Disabled);
    }

    this->historicalMessage_ = this->tags.contains("historical");

    if (this->tags.contains("msg-id") &&
        this->tags["msg-id"].toString().split(';').contains(
            "highlighted-message"))
    {
        this->message().flags.set(MessageFlag::RedeemedHighlight);
    }

    if (this->tags.contains("first-msg") &&
        this->tags["first-msg"].toString() == "1")
    {
        this->message().flags.set(MessageFlag::FirstMessage);
    }

    if (this->tags.contains("pinned-chat-paid-amount"))
    {
        this->message().flags.set(MessageFlag::ElevatedMessage);
    }

    // reply threads
    if (this->thread_)
    {
        // set references
        this->message().replyThread = this->thread_;
        this->thread_->addToThread(this->weakOf());

        // enable reply flag
        this->message().flags.set(MessageFlag::ReplyMessage);

        const auto &threadRoot = this->thread_->root();

        QString usernameText =
            stylizeUsername(threadRoot->loginName, *threadRoot.get());

        this->emplace<ReplyCurveElement>();

        // construct reply elements
        this->emplace<TextElement>(
                "Replying to", MessageElementFlag::RepliedMessage,
                MessageColor::System, FontStyle::ChatMediumSmall)
            ->setLink({Link::ViewThread, this->thread_->rootId()});

        this->emplace<TextElement>(
                "@" + usernameText + ":", MessageElementFlag::RepliedMessage,
                threadRoot->usernameColor, FontStyle::ChatMediumSmall)
            ->setLink({Link::UserInfo, threadRoot->displayName});

        this->emplace<SingleLineTextElement>(
                threadRoot->messageText, MessageElementFlag::RepliedMessage,
                this->textColor_, FontStyle::ChatMediumSmall)
            ->setLink({Link::ViewThread, this->thread_->rootId()});
    }
    else if (this->tags.find("reply-parent-msg-id") != this->tags.end())
    {
        // Message is a reply but we couldn't find the original message.
        // Render the message using the additional reply tags

        auto replyDisplayName = this->tags.find("reply-parent-display-name");
        auto replyBody = this->tags.find("reply-parent-msg-body");

        if (replyDisplayName != this->tags.end() &&
            replyBody != this->tags.end())
        {
            auto name = replyDisplayName->toString();
            auto body = parseTagString(replyBody->toString());

            this->emplace<ReplyCurveElement>();

            this->emplace<TextElement>(
                "Replying to", MessageElementFlag::RepliedMessage,
                MessageColor::System, FontStyle::ChatMediumSmall);

            this->emplace<TextElement>(
                    "@" + name + ":", MessageElementFlag::RepliedMessage,
                    this->textColor_, FontStyle::ChatMediumSmall)
                ->setLink({Link::UserInfo, name});

            this->emplace<SingleLineTextElement>(
                body, MessageElementFlag::RepliedMessage, this->textColor_,
                FontStyle::ChatMediumSmall);
        }
    }

    // timestamp
    this->message().serverReceivedTime = calculateMessageTime(this->ircMessage);
    this->emplace<TimestampElement>(this->message().serverReceivedTime.time());

    if (this->shouldAddModerationElements())
    {
        this->emplace<TwitchModerationElement>();
    }

    this->appendTwitchBadges();

    this->appendChatterinoBadges();
    this->appendFfzBadges();
    this->appendSeventvBadges();

    this->appendUsername();

    //    QString bits;
    auto iterator = this->tags.find("bits");
    if (iterator != this->tags.end())
    {
        this->hasBits_ = true;
        this->bitsLeft = iterator.value().toInt();
        this->bits = iterator.value().toString();
    }

    // Twitch emotes
    std::vector<TwitchEmoteOccurence> twitchEmotes;

    iterator = this->tags.find("emotes");
    if (iterator != this->tags.end())
    {
        QStringList emoteString = iterator.value().toString().split('/');
        std::vector<int> correctPositions;
        for (int i = 0; i < this->originalMessage_.size(); ++i)
        {
            if (!this->originalMessage_.at(i).isLowSurrogate())
            {
                correctPositions.push_back(i);
            }
        }
        for (QString emote : emoteString)
        {
            this->appendTwitchEmote(emote, twitchEmotes, correctPositions);
        }
    }

    // This runs through all ignored phrases and runs its replacements on this->originalMessage_
    this->runIgnoreReplaces(twitchEmotes);

    std::sort(twitchEmotes.begin(), twitchEmotes.end(),
              [](const auto &a, const auto &b) {
                  return a.start < b.start;
              });
    twitchEmotes.erase(std::unique(twitchEmotes.begin(), twitchEmotes.end(),
                                   [](const auto &first, const auto &second) {
                                       return first.start == second.start;
                                   }),
                       twitchEmotes.end());

    // words
    QStringList splits = this->originalMessage_.split(' ');

    this->addWords(splits, twitchEmotes);

    this->message().messageText = this->originalMessage_;
    this->message().searchText = this->message().localizedName + " " +
                                 this->userName + ": " + this->originalMessage_;

    // highlights
    this->parseHighlights();

    // highlighting incoming whispers if requested per setting
    if (this->args.isReceivedWhisper && getSettings()->highlightInlineWhispers)
    {
        this->message().flags.set(MessageFlag::HighlightedWhisper, true);
        this->message().highlightColor =
            ColorProvider::instance().color(ColorType::Whisper);
    }

    if (this->thread_)
    {
        auto &img = getResources().buttons.replyThreadDark;
        this->emplace<CircularImageElement>(
                Image::fromResourcePixmap(img, 0.15), 2, Qt::gray,
                MessageElementFlag::ReplyButton)
            ->setLink({Link::ViewThread, this->thread_->rootId()});
    }
    else
    {
        auto &img = getResources().buttons.replyDark;
        this->emplace<CircularImageElement>(
                Image::fromResourcePixmap(img, 0.15), 2, Qt::gray,
                MessageElementFlag::ReplyButton)
            ->setLink({Link::ReplyToMessage, this->message().id});
    }

    return this->release();
}

bool doesWordContainATwitchEmote(
    int cursor, const QString &word,
    const std::vector<TwitchEmoteOccurence> &twitchEmotes,
    std::vector<TwitchEmoteOccurence>::const_iterator &currentTwitchEmoteIt)
{
    if (currentTwitchEmoteIt == twitchEmotes.end())
    {
        // No emote to add!
        return false;
    }

    const auto &currentTwitchEmote = *currentTwitchEmoteIt;

    auto wordEnd = cursor + word.length();

    // Check if this emote fits within the word boundaries
    if (currentTwitchEmote.start < cursor || currentTwitchEmote.end > wordEnd)
    {
        // this emote does not fit xd
        return false;
    }

    return true;
}

void TwitchMessageBuilder::addWords(
    const QStringList &words,
    const std::vector<TwitchEmoteOccurence> &twitchEmotes)
{
    // cursor currently indicates what character index we're currently operating in the full list of words
    int cursor = 0;
    auto currentTwitchEmoteIt = twitchEmotes.begin();

    for (auto word : words)
    {
        if (word.isEmpty())
        {
            cursor++;
            continue;
        }

        while (doesWordContainATwitchEmote(cursor, word, twitchEmotes,
                                           currentTwitchEmoteIt))
        {
            const auto &currentTwitchEmote = *currentTwitchEmoteIt;

            if (currentTwitchEmote.start == cursor)
            {
                // This emote exists right at the start of the word!
                this->emplace<EmoteElement>(currentTwitchEmote.ptr,
                                            MessageElementFlag::TwitchEmote,
                                            this->textColor_);

                auto len = currentTwitchEmote.name.string.length();
                cursor += len;
                word = word.mid(len);

                ++currentTwitchEmoteIt;

                if (word.isEmpty())
                {
                    // space
                    cursor += 1;
                    break;
                }
                else
                {
                    this->message().elements.back()->setTrailingSpace(false);
                }

                continue;
            }

            // Emote is not at the start

            // 1. Add text before the emote
            QString preText = word.left(currentTwitchEmote.start - cursor);
            for (auto &variant : getApp()->emotes->emojis.parse(preText))
            {
                boost::apply_visitor(
                    [&](auto &&arg) {
                        this->addTextOrEmoji(arg);
                    },
                    variant);
            }

            cursor += preText.size();

            word = word.mid(preText.size());
        }

        if (word.isEmpty())
        {
            continue;
        }

        // split words
        for (auto &variant : getApp()->emotes->emojis.parse(word))
        {
            boost::apply_visitor(
                [&](auto &&arg) {
                    this->addTextOrEmoji(arg);
                },
                variant);
        }

        cursor += word.size() + 1;
    }
}

void TwitchMessageBuilder::addTextOrEmoji(EmotePtr emote)
{
    return SharedMessageBuilder::addTextOrEmoji(emote);
}

void TwitchMessageBuilder::addTextOrEmoji(const QString &string_)
{
    auto string = QString(string_);

    if (this->hasBits_ && this->tryParseCheermote(string))
    {
        // This string was parsed as a cheermote
        return;
    }

    // TODO: Implement ignored emotes
    // Format of ignored emotes:
    // Emote name: "forsenPuke" - if string in ignoredEmotes
    // Will match emote regardless of source (i.e. bttv, ffz)
    // Emote source + name: "bttv:nyanPls"
    if (this->tryAppendEmote({string}))
    {
        // Successfully appended an emote
        return;
    }

    // Actually just text
    auto linkString = this->matchLink(string);
    auto textColor = this->textColor_;

    if (!linkString.isEmpty())
    {
        this->addLink(string, linkString);
        return;
    }

    if (string.startsWith('@'))
    {
        auto match = mentionRegex.match(string);
        // Only treat as @mention if valid username
        if (match.hasMatch())
        {
            QString username = match.captured(1);
            auto originalTextColor = textColor;

            if (this->twitchChannel != nullptr && getSettings()->colorUsernames)
            {
                if (auto userColor =
                        this->twitchChannel->getUserColor(username);
                    userColor.isValid())
                {
                    textColor = userColor;
                }
            }

            auto prefixedUsername = '@' + username;
            this->emplace<TextElement>(prefixedUsername,
                                       MessageElementFlag::BoldUsername,
                                       textColor, FontStyle::ChatMediumBold)
                ->setLink({Link::UserInfo, username})
                ->setTrailingSpace(false);

            this->emplace<TextElement>(prefixedUsername,
                                       MessageElementFlag::NonBoldUsername,
                                       textColor)
                ->setLink({Link::UserInfo, username})
                ->setTrailingSpace(false);

            this->emplace<TextElement>(string.remove(prefixedUsername),
                                       MessageElementFlag::Text,
                                       originalTextColor);

            return;
        }
    }

    if (this->twitchChannel != nullptr && getSettings()->findAllUsernames)
    {
        auto match = allUsernamesMentionRegex.match(string);
        QString username = match.captured(1);

        if (match.hasMatch() &&
            this->twitchChannel->accessChatters()->contains(username))
        {
            auto originalTextColor = textColor;

            if (getSettings()->colorUsernames)
            {
                if (auto userColor =
                        this->twitchChannel->getUserColor(username);
                    userColor.isValid())
                {
                    textColor = userColor;
                }
            }

            this->emplace<TextElement>(username,
                                       MessageElementFlag::BoldUsername,
                                       textColor, FontStyle::ChatMediumBold)
                ->setLink({Link::UserInfo, username})
                ->setTrailingSpace(false);

            this->emplace<TextElement>(
                    username, MessageElementFlag::NonBoldUsername, textColor)
                ->setLink({Link::UserInfo, username})
                ->setTrailingSpace(false);

            this->emplace<TextElement>(string.remove(username),
                                       MessageElementFlag::Text,
                                       originalTextColor);

            return;
        }
    }

    this->emplace<TextElement>(string, MessageElementFlag::Text, textColor);
}

void TwitchMessageBuilder::parseMessageID()
{
    auto iterator = this->tags.find("id");

    if (iterator != this->tags.end())
    {
        this->message().id = iterator.value().toString();
    }
}

void TwitchMessageBuilder::parseRoomID()
{
    if (this->twitchChannel == nullptr)
    {
        return;
    }

    auto iterator = this->tags.find("room-id");

    if (iterator != std::end(this->tags))
    {
        this->roomID_ = iterator.value().toString();

        if (this->twitchChannel->roomId().isEmpty())
        {
            this->twitchChannel->setRoomId(this->roomID_);
        }
    }
}

void TwitchMessageBuilder::parseUsernameColor()
{
    const auto iterator = this->tags.find("color");
    if (iterator != this->tags.end())
    {
        if (const auto color = iterator.value().toString(); !color.isEmpty())
        {
            this->usernameColor_ = QColor(color);
            this->message().usernameColor = this->usernameColor_;
            return;
        }
    }

    if (getSettings()->colorizeNicknames && this->tags.contains("user-id"))
    {
        this->usernameColor_ =
            getRandomColor(this->tags.value("user-id").toString());
        this->message().usernameColor = this->usernameColor_;
    }
}

void TwitchMessageBuilder::parseUsername()
{
    SharedMessageBuilder::parseUsername();

    if (this->userName.isEmpty() || this->args.trimSubscriberUsername)
    {
        this->userName = this->tags.value(QLatin1String("login")).toString();
    }

    // display name
    //    auto displayNameVariant = this->tags.value("display-name");
    //    if (displayNameVariant.isValid()) {
    //        this->userName = displayNameVariant.toString() + " (" +
    //        this->userName + ")";
    //    }

    this->message().loginName = this->userName;
    if (this->twitchChannel != nullptr)
    {
        this->twitchChannel->setUserColor(this->userName, this->usernameColor_);
    }

    // Update current user color if this is our message
    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (this->ircMessage->nick() == currentUser->getUserName())
    {
        currentUser->setColor(this->usernameColor_);
    }
}

void TwitchMessageBuilder::appendUsername()
{
    auto app = getApp();

    QString username = this->userName;
    this->message().loginName = username;
    QString localizedName;

    auto iterator = this->tags.find("display-name");
    if (iterator != this->tags.end())
    {
        QString displayName =
            parseTagString(iterator.value().toString()).trimmed();

        if (QString::compare(displayName, this->userName,
                             Qt::CaseInsensitive) == 0)
        {
            username = displayName;

            this->message().displayName = displayName;
        }
        else
        {
            localizedName = displayName;

            this->message().displayName = username;
            this->message().localizedName = displayName;
        }
    }

    QString usernameText = stylizeUsername(username, this->message());

    if (this->args.isSentWhisper)
    {
        // TODO(pajlada): Re-implement
        // userDisplayString +=
        // IrcManager::instance().getUser().getUserName();
    }
    else if (this->args.isReceivedWhisper)
    {
        // Sender username
        this->emplace<TextElement>(usernameText, MessageElementFlag::Username,
                                   this->usernameColor_,
                                   FontStyle::ChatMediumBold)
            ->setLink({Link::UserWhisper, this->message().displayName});

        auto currentUser = app->accounts->twitch.getCurrent();

        // Separator
        this->emplace<TextElement>("->", MessageElementFlag::Username,
                                   MessageColor::System, FontStyle::ChatMedium);

        QColor selfColor = currentUser->color();
        MessageColor selfMsgColor =
            selfColor.isValid() ? selfColor : MessageColor::System;

        // Your own username
        this->emplace<TextElement>(currentUser->getUserName() + ":",
                                   MessageElementFlag::Username, selfMsgColor,
                                   FontStyle::ChatMediumBold);
    }
    else
    {
        if (!this->action_)
        {
            usernameText += ":";
        }

        this->emplace<TextElement>(usernameText, MessageElementFlag::Username,
                                   this->usernameColor_,
                                   FontStyle::ChatMediumBold)
            ->setLink({Link::UserInfo, this->message().displayName});
    }
}

void TwitchMessageBuilder::runIgnoreReplaces(
    std::vector<TwitchEmoteOccurence> &twitchEmotes)
{
    auto phrases = getCSettings().ignoredMessages.readOnly();
    auto removeEmotesInRange = [](int pos, int len,
                                  auto &twitchEmotes) mutable {
        auto it = std::partition(
            twitchEmotes.begin(), twitchEmotes.end(),
            [pos, len](const auto &item) {
                return !((item.start >= pos) && item.start < (pos + len));
            });
        for (auto copy = it; copy != twitchEmotes.end(); ++copy)
        {
            if ((*copy).ptr == nullptr)
            {
                qCDebug(chatterinoTwitch)
                    << "remem nullptr" << (*copy).name.string;
            }
        }
        std::vector<TwitchEmoteOccurence> v(it, twitchEmotes.end());
        twitchEmotes.erase(it, twitchEmotes.end());
        return v;
    };

    auto shiftIndicesAfter = [&twitchEmotes](int pos, int by) mutable {
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
                                         const QStringRef &midrepl,
                                         int startIndex) mutable {
        if (!phrase.containsEmote())
        {
            return;
        }

        QVector<QStringRef> words = midrepl.split(' ');
        int pos = 0;
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
                    twitchEmotes.push_back(TwitchEmoteOccurence{
                        startIndex + pos,
                        startIndex + pos + emote.first.string.length(),
                        emote.second,
                        emote.first,
                    });
                }
            }
            pos += word.length() + 1;
        }
    };

    for (const auto &phrase : *phrases)
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
            int from = 0;
            while ((from = this->originalMessage_.indexOf(regex, from,
                                                          &match)) != -1)
            {
                int len = match.capturedLength();
                auto vret = removeEmotesInRange(from, len, twitchEmotes);
                auto mid = this->originalMessage_.mid(from, len);
                mid.replace(regex, phrase.getReplace());

                int midsize = mid.size();
                this->originalMessage_.replace(from, len, mid);
                int pos1 = from;
                while (pos1 > 0)
                {
                    if (this->originalMessage_[pos1 - 1] == ' ')
                    {
                        break;
                    }
                    --pos1;
                }
                int pos2 = from + midsize;
                while (pos2 < this->originalMessage_.length())
                {
                    if (this->originalMessage_[pos2] == ' ')
                    {
                        break;
                    }
                    ++pos2;
                }

                shiftIndicesAfter(from + len, midsize - len);

                auto midExtendedRef =
                    this->originalMessage_.midRef(pos1, pos2 - pos1);

                for (auto &tup : vret)
                {
                    if (tup.ptr == nullptr)
                    {
                        qCDebug(chatterinoTwitch)
                            << "v nullptr" << tup.name.string;
                        continue;
                    }
                    QRegularExpression emoteregex(
                        "\\b" + tup.name.string + "\\b",
                        QRegularExpression::UseUnicodePropertiesOption);
                    auto _match = emoteregex.match(midExtendedRef);
                    if (_match.hasMatch())
                    {
                        int last = _match.lastCapturedIndex();
                        for (int i = 0; i <= last; ++i)
                        {
                            tup.start = from + _match.capturedStart();
                            twitchEmotes.push_back(std::move(tup));
                        }
                    }
                }

                addReplEmotes(phrase, midExtendedRef, pos1);

                from += midsize;
            }
        }
        else
        {
            int from = 0;
            while ((from = this->originalMessage_.indexOf(
                        pattern, from, phrase.caseSensitivity())) != -1)
            {
                int len = pattern.size();
                auto vret = removeEmotesInRange(from, len, twitchEmotes);
                auto replace = phrase.getReplace();

                int replacesize = replace.size();
                this->originalMessage_.replace(from, len, replace);

                int pos1 = from;
                while (pos1 > 0)
                {
                    if (this->originalMessage_[pos1 - 1] == ' ')
                    {
                        break;
                    }
                    --pos1;
                }
                int pos2 = from + replacesize;
                while (pos2 < this->originalMessage_.length())
                {
                    if (this->originalMessage_[pos2] == ' ')
                    {
                        break;
                    }
                    ++pos2;
                }

                shiftIndicesAfter(from + len, replacesize - len);

                auto midExtendedRef =
                    this->originalMessage_.midRef(pos1, pos2 - pos1);

                for (auto &tup : vret)
                {
                    if (tup.ptr == nullptr)
                    {
                        qCDebug(chatterinoTwitch)
                            << "v nullptr" << tup.name.string;
                        continue;
                    }
                    QRegularExpression emoteregex(
                        "\\b" + tup.name.string + "\\b",
                        QRegularExpression::UseUnicodePropertiesOption);
                    auto match = emoteregex.match(midExtendedRef);
                    if (match.hasMatch())
                    {
                        int last = match.lastCapturedIndex();
                        for (int i = 0; i <= last; ++i)
                        {
                            tup.start = from + match.capturedStart();
                            twitchEmotes.push_back(std::move(tup));
                        }
                    }
                }

                addReplEmotes(phrase, midExtendedRef, pos1);

                from += replacesize;
            }
        }
    }
}

void TwitchMessageBuilder::appendTwitchEmote(
    const QString &emote, std::vector<TwitchEmoteOccurence> &vec,
    std::vector<int> &correctPositions)
{
    auto app = getApp();
    if (!emote.contains(':'))
    {
        return;
    }

    auto parameters = emote.split(':');

    if (parameters.length() < 2)
    {
        return;
    }

    auto id = EmoteId{parameters.at(0)};

    auto occurences = parameters.at(1).split(',');

    for (QString occurence : occurences)
    {
        auto coords = occurence.split('-');

        if (coords.length() < 2)
        {
            return;
        }

        auto start =
            correctPositions[coords.at(0).toUInt() - this->messageOffset_];
        auto end =
            correctPositions[coords.at(1).toUInt() - this->messageOffset_];

        if (start >= end || start < 0 || end > this->originalMessage_.length())
        {
            return;
        }

        auto name =
            EmoteName{this->originalMessage_.mid(start, end - start + 1)};
        TwitchEmoteOccurence emoteOccurence{
            start, end, app->emotes->twitch.getOrCreateEmote(id, name), name};
        if (emoteOccurence.ptr == nullptr)
        {
            qCDebug(chatterinoTwitch)
                << "nullptr" << emoteOccurence.name.string;
        }
        vec.push_back(std::move(emoteOccurence));
    }
}

Outcome TwitchMessageBuilder::tryAppendEmote(const EmoteName &name)
{
    auto *app = getApp();

    const auto &globalBttvEmotes = app->twitch->getBttvEmotes();
    const auto &globalFfzEmotes = app->twitch->getFfzEmotes();
    const auto &globalSeventvEmotes = app->twitch->getSeventvEmotes();

    auto flags = MessageElementFlags();
    auto emote = boost::optional<EmotePtr>{};

    // Emote order:
    //  - FrankerFaceZ Channel
    //  - BetterTTV Channel
    //  - 7TV Channel
    //  - FrankerFaceZ Global
    //  - BetterTTV Global
    //  - 7TV Global
    if (this->twitchChannel && (emote = this->twitchChannel->ffzEmote(name)))
    {
        flags = MessageElementFlag::FfzEmote;
    }
    else if (this->twitchChannel &&
             (emote = this->twitchChannel->bttvEmote(name)))
    {
        flags = MessageElementFlag::BttvEmote;
    }
    else if (this->twitchChannel != nullptr &&
             (emote = this->twitchChannel->seventvEmote(name)))
    {
        flags = MessageElementFlag::SevenTVEmote;
        if (emote.value()->zeroWidth)
        {
            flags.set(MessageElementFlag::ZeroWidthEmote);
        }
    }
    else if ((emote = globalFfzEmotes.emote(name)))
    {
        flags = MessageElementFlag::FfzEmote;
    }
    else if ((emote = globalBttvEmotes.emote(name)))
    {
        flags = MessageElementFlag::BttvEmote;

        if (zeroWidthEmotes.contains(name.string))
        {
            flags.set(MessageElementFlag::ZeroWidthEmote);
        }
    }
    else if ((emote = globalSeventvEmotes.globalEmote(name)))
    {
        flags = MessageElementFlag::SevenTVEmote;
        if (emote.value()->zeroWidth)
        {
            flags.set(MessageElementFlag::ZeroWidthEmote);
        }
    }

    if (emote)
    {
        this->emplace<EmoteElement>(emote.get(), flags, this->textColor_);
        return Success;
    }

    return Failure;
}

boost::optional<EmotePtr> TwitchMessageBuilder::getTwitchBadge(
    const Badge &badge)
{
    if (auto channelBadge =
            this->twitchChannel->twitchBadge(badge.key_, badge.value_))
    {
        return channelBadge;
    }

    if (auto globalBadge =
            TwitchBadges::instance()->badge(badge.key_, badge.value_))
    {
        return globalBadge;
    }

    return boost::none;
}

std::unordered_map<QString, QString> TwitchMessageBuilder::parseBadgeInfoTag(
    const QVariantMap &tags)
{
    std::unordered_map<QString, QString> infoMap;

    auto infoIt = tags.constFind("badge-info");
    if (infoIt == tags.end())
        return infoMap;

    auto info = infoIt.value().toString().split(',', Qt::SkipEmptyParts);

    for (const QString &badge : info)
    {
        infoMap.emplace(SharedMessageBuilder::slashKeyValue(badge));
    }

    return infoMap;
}

void TwitchMessageBuilder::appendTwitchBadges()
{
    if (this->twitchChannel == nullptr)
    {
        return;
    }

    auto badgeInfos = TwitchMessageBuilder::parseBadgeInfoTag(this->tags);
    auto badges = this->parseBadgeTag(this->tags);

    for (const auto &badge : badges)
    {
        auto badgeEmote = this->getTwitchBadge(badge);
        if (!badgeEmote)
        {
            continue;
        }
        auto tooltip = (*badgeEmote)->tooltip.string;

        if (badge.key_ == "bits")
        {
            const auto &cheerAmount = badge.value_;
            tooltip = QString("Twitch cheer %0").arg(cheerAmount);
        }
        else if (badge.key_ == "moderator" &&
                 getSettings()->useCustomFfzModeratorBadges)
        {
            if (auto customModBadge = this->twitchChannel->ffzCustomModBadge())
            {
                this->emplace<ModBadgeElement>(
                        customModBadge.get(),
                        MessageElementFlag::BadgeChannelAuthority)
                    ->setTooltip((*customModBadge)->tooltip.string);
                // early out, since we have to add a custom badge element here
                continue;
            }
        }
        else if (badge.key_ == "vip" && getSettings()->useCustomFfzVipBadges)
        {
            if (auto customVipBadge = this->twitchChannel->ffzCustomVipBadge())
            {
                this->emplace<VipBadgeElement>(
                        customVipBadge.get(),
                        MessageElementFlag::BadgeChannelAuthority)
                    ->setTooltip((*customVipBadge)->tooltip.string);
                // early out, since we have to add a custom badge element here
                continue;
            }
        }
        else if (badge.flag_ == MessageElementFlag::BadgeSubscription)
        {
            auto badgeInfoIt = badgeInfos.find(badge.key_);
            if (badgeInfoIt != badgeInfos.end())
            {
                // badge.value_ is 4 chars long if user is subbed on higher tier
                // (tier + amount of months with leading zero if less than 100)
                // e.g. 3054 - tier 3 4,5-year sub. 2108 - tier 2 9-year sub
                const auto &subTier =
                    badge.value_.length() > 3 ? badge.value_.at(0) : '1';
                const auto &subMonths = badgeInfoIt->second;
                tooltip +=
                    QString(" (%1%2 months)")
                        .arg(subTier != '1' ? QString("Tier %1, ").arg(subTier)
                                            : "")
                        .arg(subMonths);
            }
        }
        else if (badge.flag_ == MessageElementFlag::BadgePredictions)
        {
            auto badgeInfoIt = badgeInfos.find(badge.key_);
            if (badgeInfoIt != badgeInfos.end())
            {
                auto predictionText =
                    badgeInfoIt->second
                        .replace(R"(\s)", " ")  // standard IRC escapes
                        .replace(R"(\:)", ";")
                        .replace(R"(\\)", R"(\)")
                        .replace("⸝", ",");  // twitch's comma escape
                // Careful, the first character is RIGHT LOW PARAPHRASE BRACKET or U+2E1D, which just looks like a comma

                tooltip = QString("Predicted %1").arg(predictionText);
            }
        }

        this->emplace<BadgeElement>(badgeEmote.get(), badge.flag_)
            ->setTooltip(tooltip);
    }

    this->message().badges = badges;
    this->message().badgeInfos = badgeInfos;
}

void TwitchMessageBuilder::appendChatterinoBadges()
{
    if (auto badge = getApp()->chatterinoBadges->getBadge({this->userId_}))
    {
        this->emplace<BadgeElement>(*badge,
                                    MessageElementFlag::BadgeChatterino);
    }
}

void TwitchMessageBuilder::appendFfzBadges()
{
    for (const auto &badge :
         getApp()->ffzBadges->getUserBadges({this->userId_}))
    {
        this->emplace<FfzBadgeElement>(
            badge.emote, MessageElementFlag::BadgeFfz, badge.color);
    }
}

void TwitchMessageBuilder::appendSeventvBadges()
{
    if (auto badge = getApp()->seventvBadges->getBadge({this->userId_}))
    {
        this->emplace<BadgeElement>(*badge, MessageElementFlag::BadgeSevenTV);
    }
}

Outcome TwitchMessageBuilder::tryParseCheermote(const QString &string)
{
    if (this->bitsLeft == 0)
    {
        return Failure;
    }

    auto cheerOpt = this->twitchChannel->cheerEmote(string);

    if (!cheerOpt)
    {
        return Failure;
    }

    auto &cheerEmote = *cheerOpt;
    auto match = cheerEmote.regex.match(string);

    if (!match.hasMatch())
    {
        return Failure;
    }

    int cheerValue = match.captured(1).toInt();

    if (getSettings()->stackBits)
    {
        if (this->bitsStacked)
        {
            return Success;
        }
        if (cheerEmote.staticEmote)
        {
            this->emplace<EmoteElement>(cheerEmote.staticEmote,
                                        MessageElementFlag::BitsStatic,
                                        this->textColor_);
        }
        if (cheerEmote.animatedEmote)
        {
            this->emplace<EmoteElement>(cheerEmote.animatedEmote,
                                        MessageElementFlag::BitsAnimated,
                                        this->textColor_);
        }
        if (cheerEmote.color != QColor())
        {
            this->emplace<TextElement>(QString::number(this->bitsLeft),
                                       MessageElementFlag::BitsAmount,
                                       cheerEmote.color);
        }
        this->bitsStacked = true;
        return Success;
    }

    if (this->bitsLeft >= cheerValue)
    {
        this->bitsLeft -= cheerValue;
    }
    else
    {
        QString newString = string;
        newString.chop(QString::number(cheerValue).length());
        newString += QString::number(cheerValue - this->bitsLeft);

        return tryParseCheermote(newString);
    }

    if (cheerEmote.staticEmote)
    {
        this->emplace<EmoteElement>(cheerEmote.staticEmote,
                                    MessageElementFlag::BitsStatic,
                                    this->textColor_);
    }
    if (cheerEmote.animatedEmote)
    {
        this->emplace<EmoteElement>(cheerEmote.animatedEmote,
                                    MessageElementFlag::BitsAnimated,
                                    this->textColor_);
    }
    if (cheerEmote.color != QColor())
    {
        this->emplace<TextElement>(match.captured(1),
                                   MessageElementFlag::BitsAmount,
                                   cheerEmote.color);
    }

    return Success;
}

bool TwitchMessageBuilder::shouldAddModerationElements() const
{
    if (this->senderIsBroadcaster)
    {
        // You cannot timeout the broadcaster
        return false;
    }

    if (this->tags.value("user-type").toString() == "mod" &&
        !this->args.isStaffOrBroadcaster)
    {
        // You cannot timeout moderators UNLESS you are Twitch Staff or the broadcaster of the channel
        return false;
    }

    return true;
}

void TwitchMessageBuilder::appendChannelPointRewardMessage(
    const ChannelPointReward &reward, MessageBuilder *builder, bool isMod,
    bool isBroadcaster)
{
    if (isIgnoredMessage({
            /*.message = */ "",
            /*.twitchUserID = */ reward.user.id,
            /*.isMod = */ isMod,
            /*.isBroadcaster = */ isBroadcaster,
        }))
    {
        return;
    }

    builder->emplace<TimestampElement>();
    QString redeemed = "Redeemed";
    QStringList textList;
    if (!reward.isUserInputRequired)
    {
        builder
            ->emplace<TextElement>(
                reward.user.login, MessageElementFlag::ChannelPointReward,
                MessageColor::Text, FontStyle::ChatMediumBold)
            ->setLink({Link::UserInfo, reward.user.login});
        redeemed = "redeemed";
        textList.append(reward.user.login);
    }
    builder->emplace<TextElement>(redeemed,
                                  MessageElementFlag::ChannelPointReward);
    builder->emplace<TextElement>(
        reward.title, MessageElementFlag::ChannelPointReward,
        MessageColor::Text, FontStyle::ChatMediumBold);
    builder->emplace<ScalingImageElement>(
        reward.image, MessageElementFlag::ChannelPointRewardImage);
    builder->emplace<TextElement>(
        QString::number(reward.cost), MessageElementFlag::ChannelPointReward,
        MessageColor::Text, FontStyle::ChatMediumBold);
    if (reward.isUserInputRequired)
    {
        builder->emplace<LinebreakElement>(
            MessageElementFlag::ChannelPointReward);
    }

    builder->message().flags.set(MessageFlag::RedeemedChannelPointReward);

    textList.append({redeemed, reward.title, QString::number(reward.cost)});
    builder->message().messageText = textList.join(" ");
    builder->message().searchText = textList.join(" ");
}

void TwitchMessageBuilder::liveMessage(const QString &channelName,
                                       MessageBuilder *builder)
{
    builder->emplace<TimestampElement>();
    builder
        ->emplace<TextElement>(channelName, MessageElementFlag::Username,
                               MessageColor::Text, FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, channelName});
    builder->emplace<TextElement>("is live!", MessageElementFlag::Text,
                                  MessageColor::Text);
    auto text = QString("%1 is live!").arg(channelName);
    builder->message().messageText = text;
    builder->message().searchText = text;
}

void TwitchMessageBuilder::liveSystemMessage(const QString &channelName,
                                             MessageBuilder *builder)
{
    builder->emplace<TimestampElement>();
    builder->message().flags.set(MessageFlag::System);
    builder->message().flags.set(MessageFlag::DoNotTriggerNotification);
    builder
        ->emplace<TextElement>(channelName, MessageElementFlag::Username,
                               MessageColor::System, FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, channelName});
    builder->emplace<TextElement>("is live!", MessageElementFlag::Text,
                                  MessageColor::System);
    auto text = QString("%1 is live!").arg(channelName);
    builder->message().messageText = text;
    builder->message().searchText = text;
}

void TwitchMessageBuilder::offlineSystemMessage(const QString &channelName,
                                                MessageBuilder *builder)
{
    builder->emplace<TimestampElement>();
    builder->message().flags.set(MessageFlag::System);
    builder->message().flags.set(MessageFlag::DoNotTriggerNotification);
    builder
        ->emplace<TextElement>(channelName, MessageElementFlag::Username,
                               MessageColor::System, FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, channelName});
    builder->emplace<TextElement>("is now offline.", MessageElementFlag::Text,
                                  MessageColor::System);
    auto text = QString("%1 is now offline.").arg(channelName);
    builder->message().messageText = text;
    builder->message().searchText = text;
}

void TwitchMessageBuilder::hostingSystemMessage(const QString &channelName,
                                                MessageBuilder *builder,
                                                bool hostOn)
{
    QString text;
    builder->emplace<TimestampElement>();
    builder->message().flags.set(MessageFlag::System);
    builder->message().flags.set(MessageFlag::DoNotTriggerNotification);
    if (hostOn)
    {
        builder->emplace<TextElement>("Now hosting", MessageElementFlag::Text,
                                      MessageColor::System);
        builder
            ->emplace<TextElement>(
                channelName + ".", MessageElementFlag::Username,
                MessageColor::System, FontStyle::ChatMediumBold)
            ->setLink({Link::UserInfo, channelName});
        text = QString("Now hosting %1.").arg(channelName);
    }
    else
    {
        builder
            ->emplace<TextElement>(channelName, MessageElementFlag::Username,
                                   MessageColor::System,
                                   FontStyle::ChatMediumBold)
            ->setLink({Link::UserInfo, channelName});
        builder->emplace<TextElement>("has gone offline. Exiting host mode.",
                                      MessageElementFlag::Text,
                                      MessageColor::System);
        text =
            QString("%1 has gone offline. Exiting host mode.").arg(channelName);
    }
    builder->message().messageText = text;
    builder->message().searchText = text;
}

// IRC variant
void TwitchMessageBuilder::deletionMessage(const MessagePtr originalMessage,
                                           MessageBuilder *builder)
{
    builder->emplace<TimestampElement>();
    builder->message().flags.set(MessageFlag::System);
    builder->message().flags.set(MessageFlag::DoNotTriggerNotification);
    builder->message().flags.set(MessageFlag::Timeout);
    // TODO(mm2pl): If or when jumping to a single message gets implemented a link,
    // add a link to the originalMessage
    builder->emplace<TextElement>("A message from", MessageElementFlag::Text,
                                  MessageColor::System);
    builder
        ->emplace<TextElement>(originalMessage->displayName,
                               MessageElementFlag::Username,
                               MessageColor::System, FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, originalMessage->loginName});
    builder->emplace<TextElement>("was deleted:", MessageElementFlag::Text,
                                  MessageColor::System);
    if (originalMessage->messageText.length() > 50)
    {
        builder
            ->emplace<TextElement>(originalMessage->messageText.left(50) + "…",
                                   MessageElementFlag::Text, MessageColor::Text)
            ->setLink({Link::JumpToMessage, originalMessage->id});
    }
    else
    {
        builder
            ->emplace<TextElement>(originalMessage->messageText,
                                   MessageElementFlag::Text, MessageColor::Text)
            ->setLink({Link::JumpToMessage, originalMessage->id});
    }
    builder->message().timeoutUser = "msg:" + originalMessage->id;
}

// pubsub variant
void TwitchMessageBuilder::deletionMessage(const DeleteAction &action,
                                           MessageBuilder *builder)
{
    builder->emplace<TimestampElement>();
    builder->message().flags.set(MessageFlag::System);
    builder->message().flags.set(MessageFlag::DoNotTriggerNotification);
    builder->message().flags.set(MessageFlag::Timeout);

    builder
        ->emplace<TextElement>(action.source.login,
                               MessageElementFlag::Username,
                               MessageColor::System, FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, action.source.login});
    // TODO(mm2pl): If or when jumping to a single message gets implemented a link,
    // add a link to the originalMessage
    builder->emplace<TextElement>(
        "deleted message from", MessageElementFlag::Text, MessageColor::System);
    builder
        ->emplace<TextElement>(action.target.login,
                               MessageElementFlag::Username,
                               MessageColor::System, FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, action.target.login});
    builder->emplace<TextElement>("saying:", MessageElementFlag::Text,
                                  MessageColor::System);
    if (action.messageText.length() > 50)
    {
        builder
            ->emplace<TextElement>(action.messageText.left(50) + "…",
                                   MessageElementFlag::Text, MessageColor::Text)
            ->setLink({Link::JumpToMessage, action.messageId});
    }
    else
    {
        builder
            ->emplace<TextElement>(action.messageText, MessageElementFlag::Text,
                                   MessageColor::Text)
            ->setLink({Link::JumpToMessage, action.messageId});
    }
    builder->message().timeoutUser = "msg:" + action.messageId;
}

void TwitchMessageBuilder::listOfUsersSystemMessage(QString prefix,
                                                    QStringList users,
                                                    Channel *channel,
                                                    MessageBuilder *builder)
{
    QString text = prefix + users.join(", ");

    builder->message().messageText = text;
    builder->message().searchText = text;

    builder->emplace<TimestampElement>();
    builder->message().flags.set(MessageFlag::System);
    builder->message().flags.set(MessageFlag::DoNotTriggerNotification);
    builder->emplace<TextElement>(prefix, MessageElementFlag::Text,
                                  MessageColor::System);
    bool isFirst = true;
    auto tc = dynamic_cast<TwitchChannel *>(channel);
    for (const QString &username : users)
    {
        if (!isFirst)
        {
            // this is used to add the ", " after each but the last entry
            builder->emplace<TextElement>(",", MessageElementFlag::Text,
                                          MessageColor::System);
        }
        isFirst = false;

        MessageColor color = MessageColor::System;

        if (tc && getSettings()->colorUsernames)
        {
            if (auto userColor = tc->getUserColor(username);
                userColor.isValid())
            {
                color = MessageColor(userColor);
            }
        }

        builder
            ->emplace<TextElement>(username, MessageElementFlag::BoldUsername,
                                   color, FontStyle::ChatMediumBold)
            ->setLink({Link::UserInfo, username})
            ->setTrailingSpace(false);
        builder
            ->emplace<TextElement>(username,
                                   MessageElementFlag::NonBoldUsername, color)
            ->setLink({Link::UserInfo, username})
            ->setTrailingSpace(false);
    }
}

void TwitchMessageBuilder::setThread(std::shared_ptr<MessageThread> thread)
{
    this->thread_ = std::move(thread);
}

void TwitchMessageBuilder::setMessageOffset(int offset)
{
    this->messageOffset_ = offset;
}

}  // namespace chatterino
