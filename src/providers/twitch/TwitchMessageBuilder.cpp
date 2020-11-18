#include "providers/twitch/TwitchMessageBuilder.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/ignores/IgnoreController.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "messages/Message.hpp"
#include "providers/chatterino/ChatterinoBadges.hpp"
#include "providers/ffz/FfzBadges.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/IrcHelpers.hpp"
#include "widgets/Window.hpp"

#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QMediaPlayer>
#include <QStringRef>
#include <boost/variant.hpp>
#include "qlogging.hpp"

namespace {

// matches a mention with punctuation at the end, like "@username," or "@username!!!" where capture group would return "username"
const QRegularExpression mentionRegex("^@(\\w+)[.,!?;]*?$");

const QSet<QString> zeroWidthEmotes{
    "SoSnowy",  "IceCold",   "SantaHat", "TopHat",
    "ReinDeer", "CandyCane", "cvMask",   "cvHazmat",
};

}  // namespace

namespace chatterino {

namespace {

    QColor getRandomColor(const QVariant &userId)
    {
        bool ok = true;
        int colorSeed = userId.toInt(&ok);
        if (!ok)
        {
            // We were unable to convert the user ID to an integer, this means Twitch has decided to start using non-integer user IDs
            // Just randomize the users color
            colorSeed = std::rand();
        }

        const auto colorIndex = colorSeed % TWITCH_USERNAME_COLORS.size();
        return TWITCH_USERNAME_COLORS[colorIndex];
    }

    QStringList parseTagList(const QVariantMap &tags, const QString &key)
    {
        auto iterator = tags.find(key);
        if (iterator == tags.end())
            return QStringList{};

        return iterator.value().toString().split(
            ',', QString::SplitBehavior::SkipEmptyParts);
    }

    std::map<QString, QString> parseBadgeInfos(const QVariantMap &tags)
    {
        std::map<QString, QString> badgeInfos;

        for (QString badgeInfo : parseTagList(tags, "badge-info"))
        {
            QStringList parts = badgeInfo.split('/');
            if (parts.size() != 2)
            {
                continue;
            }

            badgeInfos.emplace(parts[0], parts[1]);
        }

        return badgeInfos;
    }

    std::vector<Badge> parseBadges(const QVariantMap &tags)
    {
        std::vector<Badge> badges;

        for (QString badge : parseTagList(tags, "badges"))
        {
            QStringList parts = badge.split('/');
            if (parts.size() != 2)
            {
                continue;
            }

            badges.emplace_back(parts[0], parts[1]);
        }

        return badges;
    }

}  // namespace

TwitchMessageBuilder::TwitchMessageBuilder(
    Channel *_channel, const Communi::IrcPrivateMessage *_ircMessage,
    const MessageParseArgs &_args)
    : SharedMessageBuilder(_channel, _ircMessage, _args)
    , twitchChannel(dynamic_cast<TwitchChannel *>(_channel))
{
    this->usernameColor_ = getApp()->themes->messages.textColors.system;
}

TwitchMessageBuilder::TwitchMessageBuilder(
    Channel *_channel, const Communi::IrcMessage *_ircMessage,
    const MessageParseArgs &_args, QString content, bool isAction)
    : SharedMessageBuilder(_channel, _ircMessage, _args, content, isAction)
    , twitchChannel(dynamic_cast<TwitchChannel *>(_channel))
{
    this->usernameColor_ = getApp()->themes->messages.textColors.system;
}

bool TwitchMessageBuilder::isIgnored() const
{
    if (SharedMessageBuilder::isIgnored())
    {
        return true;
    }

    auto app = getApp();

    if (getSettings()->enableTwitchIgnoredUsers &&
        this->tags.contains("user-id"))
    {
        auto sourceUserID = this->tags.value("user-id").toString();

        for (const auto &user :
             app->accounts->twitch.getCurrent()->getIgnores())
        {
            if (sourceUserID == user.id)
            {
                switch (static_cast<ShowIgnoredUsersMessages>(
                    getSettings()->showIgnoredUsersMessages.getValue()))
                {
                    case ShowIgnoredUsersMessages::IfModerator:
                        if (this->channel->isMod() ||
                            this->channel->isBroadcaster())
                            return false;
                        break;
                    case ShowIgnoredUsersMessages::IfBroadcaster:
                        if (this->channel->isBroadcaster())
                            return false;
                        break;
                    case ShowIgnoredUsersMessages::Never:
                        break;
                }

                return true;
            }
        }
    }

    return false;
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
            this->appendChannelPointRewardMessage(reward.get(), this);
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

    // timestamp
    this->emplace<TimestampElement>(
        calculateMessageTimestamp(this->ircMessage));

    bool addModerationElement = true;
    if (this->senderIsBroadcaster)
    {
        addModerationElement = false;
    }
    else
    {
        bool hasUserType = this->tags.contains("user-type");
        if (hasUserType)
        {
            QString userType = this->tags.value("user-type").toString();

            if (userType == "mod")
            {
                if (!args.isStaffOrBroadcaster)
                {
                    addModerationElement = false;
                }
            }
        }
    }

    if (addModerationElement)
    {
        this->emplace<TwitchModerationElement>();
    }

    this->appendTwitchBadges();

    this->appendChatterinoBadges();
    this->appendFfzBadges();

    this->appendUsername();

    //    QString bits;
    auto iterator = this->tags.find("bits");
    if (iterator != this->tags.end())
    {
        this->hasBits_ = true;
        this->bitsLeft = iterator.value().toInt();
        this->bits = iterator.value().toString();
    }

    // twitch emotes
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
        while (doesWordContainATwitchEmote(cursor, word, twitchEmotes,
                                           currentTwitchEmoteIt))
        {
            auto wordEnd = cursor + word.length();
            const auto &currentTwitchEmote = *currentTwitchEmoteIt;

            if (currentTwitchEmote.start == cursor)
            {
                // This emote exists right at the start of the word!
                this->emplace<EmoteElement>(currentTwitchEmote.ptr,
                                            MessageElementFlag::TwitchEmote);
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
    auto textColor = this->action_ ? MessageColor(this->usernameColor_)
                                   : MessageColor(MessageColor::Text);

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
            this->emplace<TextElement>(string, MessageElementFlag::BoldUsername,
                                       textColor, FontStyle::ChatMediumBold)
                ->setLink({Link::UserInfo, username});

            this->emplace<TextElement>(
                    string, MessageElementFlag::NonBoldUsername, textColor)
                ->setLink({Link::UserInfo, username});
            return;
        }
    }

    if (this->twitchChannel != nullptr && getSettings()->findAllUsernames)
    {
        auto chatters = this->twitchChannel->accessChatters();
        if (chatters->contains(string))
        {
            this->emplace<TextElement>(string, MessageElementFlag::BoldUsername,
                                       textColor, FontStyle::ChatMediumBold)
                ->setLink({Link::UserInfo, string});

            this->emplace<TextElement>(
                    string, MessageElementFlag::NonBoldUsername, textColor)
                ->setLink({Link::UserInfo, string});
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
        this->usernameColor_ = getRandomColor(this->tags.value("user-id"));
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

    bool hasLocalizedName = !localizedName.isEmpty();

    // The full string that will be rendered in the chat widget
    QString usernameText;

    pajlada::Settings::Setting<int> usernameDisplayMode(
        "/appearance/messages/usernameDisplayMode",
        UsernameDisplayMode::UsernameAndLocalizedName);

    switch (usernameDisplayMode.getValue())
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
                                   app->themes->messages.textColors.system,
                                   FontStyle::ChatMedium);

        QColor selfColor = currentUser->color();
        if (!selfColor.isValid())
        {
            selfColor = app->themes->messages.textColors.system;
        }

        // Your own username
        this->emplace<TextElement>(currentUser->getUserName() + ":",
                                   MessageElementFlag::Username, selfColor,
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

        auto start = correctPositions[coords.at(0).toUInt()];
        auto end = correctPositions[coords.at(1).toUInt()];

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

    const auto &globalBttvEmotes = app->twitch.server->getBttvEmotes();
    const auto &globalFfzEmotes = app->twitch.server->getFfzEmotes();

    auto flags = MessageElementFlags();
    auto emote = boost::optional<EmotePtr>{};

    // Emote order:
    //  - FrankerFaceZ Channel
    //  - BetterTTV Channel
    //  - FrankerFaceZ Global
    //  - BetterTTV Global
    if (this->twitchChannel && (emote = this->twitchChannel->ffzEmote(name)))
    {
        flags = MessageElementFlag::FfzEmote;
    }
    else if (this->twitchChannel &&
             (emote = this->twitchChannel->bttvEmote(name)))
    {
        flags = MessageElementFlag::BttvEmote;
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

    if (emote)
    {
        this->emplace<EmoteElement>(emote.get(), flags);
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

    if (auto globalBadge = this->twitchChannel->globalTwitchBadges().badge(
            badge.key_, badge.value_))
    {
        return globalBadge;
    }

    return boost::none;
}

void TwitchMessageBuilder::appendTwitchBadges()
{
    if (this->twitchChannel == nullptr)
    {
        return;
    }

    auto badgeInfos = parseBadgeInfos(this->tags);
    auto badges = parseBadges(this->tags);

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
        else if (badge.key_ == "moderator")
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
        else if (badge.flag_ == MessageElementFlag::BadgeSubscription)
        {
            auto badgeInfoIt = badgeInfos.find(badge.key_);
            if (badgeInfoIt != badgeInfos.end())
            {
                // badge.value_ is 4 chars long if user is subbed on higher tier
                // (tier + amount of months with leading zero if less than 100)
                // e.g. 3054 - tier 3 4,5-year sub. 2108 - tier 2 9-year sub
                const auto &subTier =
                    badge.value_.length() > 3 ? badge.value_.front() : '1';
                const auto &subMonths = badgeInfoIt->second;
                tooltip +=
                    QString(" (%1%2 months)")
                        .arg(subTier != '1' ? QString("Tier %1, ").arg(subTier)
                                            : "")
                        .arg(subMonths);
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
    if (auto badge = getApp()->ffzBadges->getBadge({this->userId_}))
    {
        if (auto color = getApp()->ffzBadges->getBadgeColor({this->userId_}))
        {
            this->emplace<FfzBadgeElement>(*badge, MessageElementFlag::BadgeFfz,
                                           color.get());
        }
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
                                        MessageElementFlag::BitsStatic);
        }
        if (cheerEmote.animatedEmote)
        {
            this->emplace<EmoteElement>(cheerEmote.animatedEmote,
                                        MessageElementFlag::BitsAnimated);
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
                                    MessageElementFlag::BitsStatic);
    }
    if (cheerEmote.animatedEmote)
    {
        this->emplace<EmoteElement>(cheerEmote.animatedEmote,
                                    MessageElementFlag::BitsAnimated);
    }
    if (cheerEmote.color != QColor())
    {
        this->emplace<TextElement>(match.captured(1),
                                   MessageElementFlag::BitsAmount,
                                   cheerEmote.color);
    }

    return Success;
}

void TwitchMessageBuilder::appendChannelPointRewardMessage(
    const ChannelPointReward &reward, MessageBuilder *builder)
{
    builder->emplace<TimestampElement>();
    QString redeemed = "Redeemed";
    if (!reward.isUserInputRequired)
    {
        builder->emplace<TextElement>(
            reward.user.login, MessageElementFlag::ChannelPointReward,
            MessageColor::Text, FontStyle::ChatMediumBold);
        redeemed = "redeemed";
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
}

}  // namespace chatterino
