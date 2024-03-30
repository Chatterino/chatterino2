#include "providers/twitch/TwitchMessageBuilder.hpp"

#include "Application.hpp"
#include "common/LinkParser.hpp"
#include "common/Literals.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/ignores/IgnoreController.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "controllers/userdata/UserDataController.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/Message.hpp"
#include "messages/MessageThread.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/chatterino/ChatterinoBadges.hpp"
#include "providers/colors/ColorProvider.hpp"
#include "providers/ffz/FfzBadges.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/seventv/SeventvBadges.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/ChannelPointReward.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/FormatTime.hpp"
#include "util/Helpers.hpp"
#include "util/IrcHelpers.hpp"
#include "util/QStringHash.hpp"
#include "util/Qt.hpp"
#include "widgets/Window.hpp"

#include <boost/variant.hpp>
#include <QColor>
#include <QDebug>

#include <chrono>
#include <unordered_set>

using namespace chatterino::literals;

namespace {

using namespace std::chrono_literals;

const QString regexHelpString("(\\w+)[.,!?;:]*?$");

// matches a mention with punctuation at the end, like "@username," or "@username!!!" where capture group would return "username"
const QRegularExpression mentionRegex("^@" + regexHelpString);

// if findAllUsernames setting is enabled, matches strings like in the examples above, but without @ symbol at the beginning
const QRegularExpression allUsernamesMentionRegex("^" + regexHelpString);

const QSet<QString> zeroWidthEmotes{
    "SoSnowy",  "IceCold",   "SantaHat", "TopHat",
    "ReinDeer", "CandyCane", "cvMask",   "cvHazmat",
};

struct HypeChatPaidLevel {
    std::chrono::seconds duration;
    uint8_t numeric;
};

const std::unordered_map<QString, HypeChatPaidLevel> HYPE_CHAT_PAID_LEVEL{
    {u"ONE"_s, {30s, 1}},    {u"TWO"_s, {2min + 30s, 2}},
    {u"THREE"_s, {5min, 3}}, {u"FOUR"_s, {10min, 4}},
    {u"FIVE"_s, {30min, 5}}, {u"SIX"_s, {1h, 6}},
    {u"SEVEN"_s, {2h, 7}},   {u"EIGHT"_s, {3h, 8}},
    {u"NINE"_s, {4h, 9}},    {u"TEN"_s, {5h, 10}},
};

}  // namespace

namespace chatterino {

namespace {

    void appendTwitchEmoteOccurrences(const QString &emote,
                                      std::vector<TwitchEmoteOccurrence> &vec,
                                      const std::vector<int> &correctPositions,
                                      const QString &originalMessage,
                                      int messageOffset)
    {
        auto *app = getIApp();
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

        auto occurrences = parameters.at(1).split(',');

        for (const QString &occurrence : occurrences)
        {
            auto coords = occurrence.split('-');

            if (coords.length() < 2)
            {
                return;
            }

            auto from = coords.at(0).toUInt() - messageOffset;
            auto to = coords.at(1).toUInt() - messageOffset;
            auto maxPositions = correctPositions.size();
            if (from > to || to >= maxPositions)
            {
                // Emote coords are out of range
                qCDebug(chatterinoTwitch)
                    << "Emote coords" << from << "-" << to
                    << "are out of range (" << maxPositions << ")";
                return;
            }

            auto start = correctPositions[from];
            auto end = correctPositions[to];
            if (start > end || start < 0 || end > originalMessage.length())
            {
                // Emote coords are out of range from the modified character positions
                qCDebug(chatterinoTwitch) << "Emote coords" << from << "-" << to
                                          << "are out of range after offsets ("
                                          << originalMessage.length() << ")";
                return;
            }

            auto name = EmoteName{originalMessage.mid(start, end - start + 1)};
            TwitchEmoteOccurrence emoteOccurrence{
                start,
                end,
                app->getEmotes()->getTwitchEmotes()->getOrCreateEmote(id, name),
                name,
            };
            if (emoteOccurrence.ptr == nullptr)
            {
                qCDebug(chatterinoTwitch)
                    << "nullptr" << emoteOccurrence.name.string;
            }
            vec.push_back(std::move(emoteOccurrence));
        }
    }

    std::optional<EmotePtr> getTwitchBadge(const Badge &badge,
                                           const TwitchChannel *twitchChannel)
    {
        if (auto channelBadge =
                twitchChannel->twitchBadge(badge.key_, badge.value_))
        {
            return channelBadge;
        }

        if (auto globalBadge =
                getIApp()->getTwitchBadges()->badge(badge.key_, badge.value_))
        {
            return globalBadge;
        }

        return std::nullopt;
    }

    void appendBadges(MessageBuilder *builder, const std::vector<Badge> &badges,
                      const std::unordered_map<QString, QString> &badgeInfos,
                      const TwitchChannel *twitchChannel)
    {
        if (twitchChannel == nullptr)
        {
            return;
        }

        for (const auto &badge : badges)
        {
            auto badgeEmote = getTwitchBadge(badge, twitchChannel);
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
                if (auto customModBadge = twitchChannel->ffzCustomModBadge())
                {
                    builder
                        ->emplace<ModBadgeElement>(
                            *customModBadge,
                            MessageElementFlag::BadgeChannelAuthority)
                        ->setTooltip((*customModBadge)->tooltip.string);
                    // early out, since we have to add a custom badge element here
                    continue;
                }
            }
            else if (badge.key_ == "vip" &&
                     getSettings()->useCustomFfzVipBadges)
            {
                if (auto customVipBadge = twitchChannel->ffzCustomVipBadge())
                {
                    builder
                        ->emplace<VipBadgeElement>(
                            *customVipBadge,
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
                    tooltip += QString(" (%1%2 months)")
                                   .arg(subTier != '1'
                                            ? QString("Tier %1, ").arg(subTier)
                                            : "")
                                   .arg(subMonths);
                }
            }
            else if (badge.flag_ == MessageElementFlag::BadgePredictions)
            {
                auto badgeInfoIt = badgeInfos.find(badge.key_);
                if (badgeInfoIt != badgeInfos.end())
                {
                    auto infoValue = badgeInfoIt->second;
                    auto predictionText =
                        infoValue
                            .replace(R"(\s)", " ")  // standard IRC escapes
                            .replace(R"(\:)", ";")
                            .replace(R"(\\)", R"(\)")
                            .replace("⸝", ",");  // twitch's comma escape
                    // Careful, the first character is RIGHT LOW PARAPHRASE BRACKET or U+2E1D, which just looks like a comma

                    tooltip = QString("Predicted %1").arg(predictionText);
                }
            }

            builder->emplace<BadgeElement>(*badgeEmote, badge.flag_)
                ->setTooltip(tooltip);
        }

        builder->message().badges = badges;
        builder->message().badgeInfos = badgeInfos;
    }

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
                if (secondDigit != -1 &&
                    ((no * 10) + secondDigit) <= numCaptures)
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
        for (const QStringCapture &backReference :
             std::as_const(backReferences))
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
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 2)
            static_assert(sizeof(QChar) == sizeof(decltype(*chunk.utf16())));
            dst.append(reinterpret_cast<const QChar *>(chunk.utf16()),
                       chunk.length());
#else
            dst += chunk;
#endif
        }
        return dst;
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

bool TwitchMessageBuilder::isIgnoredReply() const
{
    return isIgnoredMessage({
        /*.message = */ this->originalMessage_,
        /*.twitchUserID = */
        this->tags.value("reply-parent-user-id").toString(),
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
    assert(this->ircMessage != nullptr);
    assert(this->channel != nullptr);

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
            TwitchMessageBuilder::appendChannelPointRewardMessage(
                *reward, this, this->channel->isMod(),
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

    if (this->tags.contains("bits"))
    {
        this->message().flags.set(MessageFlag::CheerMessage);
    }

    // reply threads
    this->parseThread();

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
    auto twitchEmotes = TwitchMessageBuilder::parseTwitchEmotes(
        this->tags, this->originalMessage_, this->messageOffset_);

    // This runs through all ignored phrases and runs its replacements on this->originalMessage_
    TwitchMessageBuilder::processIgnorePhrases(
        *getSettings()->ignoredMessages.readOnly(), this->originalMessage_,
        twitchEmotes);

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

    QString stylizedUsername =
        this->stylizeUsername(this->userName, this->message());

    this->message().messageText = this->originalMessage_;
    this->message().searchText =
        stylizedUsername + " " + this->message().localizedName + " " +
        this->userName + ": " + this->originalMessage_ + " " +
        this->message().searchText;

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
    const std::vector<TwitchEmoteOccurrence> &twitchEmotes,
    std::vector<TwitchEmoteOccurrence>::const_iterator &currentTwitchEmoteIt)
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
    const std::vector<TwitchEmoteOccurrence> &twitchEmotes)
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
            for (auto &variant :
                 getIApp()->getEmotes()->getEmojis()->parse(preText))
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
        for (auto &variant : getIApp()->getEmotes()->getEmojis()->parse(word))
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
    LinkParser parsed(string);
    auto textColor = this->textColor_;

    if (parsed.result())
    {
        this->addLink(*parsed.result());
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

void TwitchMessageBuilder::parseThread()
{
    if (this->thread_)
    {
        // set references
        this->message().replyThread = this->thread_;
        this->message().replyParent = this->parent_;
        this->thread_->addToThread(this->weakOf());

        // enable reply flag
        this->message().flags.set(MessageFlag::ReplyMessage);

        MessagePtr threadRoot;
        if (!this->parent_)
        {
            threadRoot = this->thread_->root();
        }
        else
        {
            threadRoot = this->parent_;
        }

        QString usernameText = SharedMessageBuilder::stylizeUsername(
            threadRoot->loginName, *threadRoot);

        this->emplace<ReplyCurveElement>();

        // construct reply elements
        this->emplace<TextElement>(
                "Replying to", MessageElementFlag::RepliedMessage,
                MessageColor::System, FontStyle::ChatMediumSmall)
            ->setLink({Link::ViewThread, this->thread_->rootId()});

        this->emplace<TextElement>(
                "@" + usernameText +
                    (threadRoot->flags.has(MessageFlag::Action) ? "" : ":"),
                MessageElementFlag::RepliedMessage, threadRoot->usernameColor,
                FontStyle::ChatMediumSmall)
            ->setLink({Link::UserInfo, threadRoot->displayName});

        MessageColor color = MessageColor::Text;
        if (threadRoot->flags.has(MessageFlag::Action))
        {
            color = threadRoot->usernameColor;
        }
        this->emplace<SingleLineTextElement>(
                threadRoot->messageText,
                MessageElementFlags({MessageElementFlag::RepliedMessage,
                                     MessageElementFlag::Text}),
                color, FontStyle::ChatMediumSmall)
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
            QString body;

            this->emplace<ReplyCurveElement>();
            this->emplace<TextElement>(
                "Replying to", MessageElementFlag::RepliedMessage,
                MessageColor::System, FontStyle::ChatMediumSmall);

            if (this->isIgnoredReply())
            {
                body = QString("[Blocked user]");
            }
            else
            {
                auto name = replyDisplayName->toString();
                body = parseTagString(replyBody->toString());

                this->emplace<TextElement>(
                        "@" + name + ":", MessageElementFlag::RepliedMessage,
                        this->textColor_, FontStyle::ChatMediumSmall)
                    ->setLink({Link::UserInfo, name});
            }

            this->emplace<SingleLineTextElement>(
                body,
                MessageElementFlags({MessageElementFlag::RepliedMessage,
                                     MessageElementFlag::Text}),
                this->textColor_, FontStyle::ChatMediumSmall);
        }
    }
}

void TwitchMessageBuilder::parseUsernameColor()
{
    const auto *userData = getIApp()->getUserData();
    assert(userData != nullptr);

    if (const auto &user = userData->getUser(this->userId_))
    {
        if (user->color)
        {
            this->usernameColor_ = user->color.value();
            return;
        }
    }

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
    auto currentUser = getIApp()->getAccounts()->twitch.getCurrent();
    if (this->ircMessage->nick() == currentUser->getUserName())
    {
        currentUser->setColor(this->usernameColor_);
    }
}

void TwitchMessageBuilder::appendUsername()
{
    auto *app = getIApp();

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

    QString usernameText =
        SharedMessageBuilder::stylizeUsername(username, this->message());

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

        auto currentUser = app->getAccounts()->twitch.getCurrent();

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

void TwitchMessageBuilder::processIgnorePhrases(
    const std::vector<IgnorePhrase> &phrases, QString &originalMessage,
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
        originalMessage.replace(from, length, replacement);
        auto wordStart = from;
        while (wordStart > 0)
        {
            if (originalMessage[wordStart - 1] == ' ')
            {
                break;
            }
            --wordStart;
        }
        auto wordEnd = from + replacement.length();
        while (wordEnd < originalMessage.length())
        {
            if (originalMessage[wordEnd] == ' ')
            {
                break;
            }
            ++wordEnd;
        }

        shiftIndicesAfter(static_cast<int>(from + length),
                          static_cast<int>(replacement.length() - length));

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        auto midExtendedRef =
            QStringView{originalMessage}.mid(wordStart, wordEnd - wordStart);
#else
        auto midExtendedRef =
            originalMessage.midRef(wordStart, wordEnd - wordStart);
#endif

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
            auto match = emoteregex.match(midExtendedRef);
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
            while ((from = originalMessage.indexOf(regex, from, &match)) != -1)
            {
                auto replacement = phrase.getReplace();
                if (regex.captureCount() > 0)
                {
                    replacement = makeRegexReplacement(originalMessage, regex,
                                                       match, replacement);
                }

                replaceMessageAt(phrase, from, match.capturedLength(),
                                 replacement);
                from += phrase.getReplace().length();
                iterations++;
                if (iterations >= 128)
                {
                    originalMessage =
                        u"Too many replacements - check your ignores!"_s;
                    return;
                }
            }

            continue;
        }

        SizeType from = 0;
        while ((from = originalMessage.indexOf(pattern, from,
                                               phrase.caseSensitivity())) != -1)
        {
            replaceMessageAt(phrase, from, pattern.length(),
                             phrase.getReplace());
            from += phrase.getReplace().length();
        }
    }
}

Outcome TwitchMessageBuilder::tryAppendEmote(const EmoteName &name)
{
    auto *app = getIApp();

    const auto *globalBttvEmotes = app->getBttvEmotes();
    const auto *globalFfzEmotes = app->getFfzEmotes();
    const auto *globalSeventvEmotes = app->getSeventvEmotes();

    auto flags = MessageElementFlags();
    auto emote = std::optional<EmotePtr>{};
    bool zeroWidth = false;

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
        zeroWidth = emote.value()->zeroWidth;
    }
    else if ((emote = globalFfzEmotes->emote(name)))
    {
        flags = MessageElementFlag::FfzEmote;
    }
    else if ((emote = globalBttvEmotes->emote(name)))
    {
        flags = MessageElementFlag::BttvEmote;
        zeroWidth = zeroWidthEmotes.contains(name.string);
    }
    else if ((emote = globalSeventvEmotes->globalEmote(name)))
    {
        flags = MessageElementFlag::SevenTVEmote;
        zeroWidth = emote.value()->zeroWidth;
    }

    if (emote)
    {
        if (zeroWidth && getSettings()->enableZeroWidthEmotes &&
            !this->isEmpty())
        {
            // Attempt to merge current zero-width emote into any previous emotes
            auto *asEmote = dynamic_cast<EmoteElement *>(&this->back());
            if (asEmote)
            {
                // Make sure to access asEmote before taking ownership when releasing
                auto baseEmote = asEmote->getEmote();
                // Need to remove EmoteElement and replace with LayeredEmoteElement
                auto baseEmoteElement = this->releaseBack();

                std::vector<LayeredEmoteElement::Emote> layers = {
                    {baseEmote, baseEmoteElement->getFlags()}, {*emote, flags}};
                this->emplace<LayeredEmoteElement>(
                    std::move(layers), baseEmoteElement->getFlags() | flags,
                    this->textColor_);
                return Success;
            }

            auto *asLayered =
                dynamic_cast<LayeredEmoteElement *>(&this->back());
            if (asLayered)
            {
                asLayered->addEmoteLayer({*emote, flags});
                asLayered->addFlags(flags);
                return Success;
            }

            // No emote to merge with, just show as regular emote
        }

        this->emplace<EmoteElement>(*emote, flags, this->textColor_);
        return Success;
    }

    return Failure;
}

std::unordered_map<QString, QString> TwitchMessageBuilder::parseBadgeInfoTag(
    const QVariantMap &tags)
{
    std::unordered_map<QString, QString> infoMap;

    auto infoIt = tags.constFind("badge-info");
    if (infoIt == tags.end())
    {
        return infoMap;
    }

    auto info = infoIt.value().toString().split(',', Qt::SkipEmptyParts);

    for (const QString &badge : info)
    {
        infoMap.emplace(SharedMessageBuilder::slashKeyValue(badge));
    }

    return infoMap;
}

std::vector<TwitchEmoteOccurrence> TwitchMessageBuilder::parseTwitchEmotes(
    const QVariantMap &tags, const QString &originalMessage, int messageOffset)
{
    // Twitch emotes
    std::vector<TwitchEmoteOccurrence> twitchEmotes;

    auto emotesTag = tags.find("emotes");

    if (emotesTag == tags.end())
    {
        return twitchEmotes;
    }

    QStringList emoteString = emotesTag.value().toString().split('/');
    std::vector<int> correctPositions;
    for (int i = 0; i < originalMessage.size(); ++i)
    {
        if (!originalMessage.at(i).isLowSurrogate())
        {
            correctPositions.push_back(i);
        }
    }
    for (const QString &emote : emoteString)
    {
        appendTwitchEmoteOccurrences(emote, twitchEmotes, correctPositions,
                                     originalMessage, messageOffset);
    }

    return twitchEmotes;
}

void TwitchMessageBuilder::appendTwitchBadges()
{
    if (this->twitchChannel == nullptr)
    {
        return;
    }

    auto badgeInfos = TwitchMessageBuilder::parseBadgeInfoTag(this->tags);
    auto badges = TwitchMessageBuilder::parseBadgeTag(this->tags);
    appendBadges(this, badges, badgeInfos, this->twitchChannel);
}

void TwitchMessageBuilder::appendChatterinoBadges()
{
    if (auto badge =
            getIApp()->getChatterinoBadges()->getBadge({this->userId_}))
    {
        this->emplace<BadgeElement>(*badge,
                                    MessageElementFlag::BadgeChatterino);
    }
}

void TwitchMessageBuilder::appendFfzBadges()
{
    for (const auto &badge :
         getIApp()->getFfzBadges()->getUserBadges({this->userId_}))
    {
        this->emplace<FfzBadgeElement>(
            badge.emote, MessageElementFlag::BadgeFfz, badge.color);
    }

    if (this->twitchChannel == nullptr)
    {
        return;
    }

    for (const auto &badge :
         this->twitchChannel->ffzChannelBadges(this->userId_))
    {
        this->emplace<FfzBadgeElement>(
            badge.emote, MessageElementFlag::BadgeFfz, badge.color);
    }
}

void TwitchMessageBuilder::appendSeventvBadges()
{
    if (auto badge = getIApp()->getSeventvBadges()->getBadge({this->userId_}))
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
    builder->message().loginName = reward.user.login;

    builder->message().reward = std::make_shared<ChannelPointReward>(reward);
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
    auto *tc = dynamic_cast<TwitchChannel *>(channel);
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

void TwitchMessageBuilder::listOfUsersSystemMessage(
    QString prefix, const std::vector<HelixModerator> &users, Channel *channel,
    MessageBuilder *builder)
{
    QString text = prefix;

    builder->emplace<TimestampElement>();
    builder->message().flags.set(MessageFlag::System);
    builder->message().flags.set(MessageFlag::DoNotTriggerNotification);
    builder->emplace<TextElement>(prefix, MessageElementFlag::Text,
                                  MessageColor::System);
    bool isFirst = true;
    auto *tc = dynamic_cast<TwitchChannel *>(channel);
    for (const auto &user : users)
    {
        if (!isFirst)
        {
            // this is used to add the ", " after each but the last entry
            builder->emplace<TextElement>(",", MessageElementFlag::Text,
                                          MessageColor::System);
            text += QString(", %1").arg(user.userName);
        }
        else
        {
            text += user.userName;
        }
        isFirst = false;

        MessageColor color = MessageColor::System;

        if (tc && getSettings()->colorUsernames)
        {
            if (auto userColor = tc->getUserColor(user.userLogin);
                userColor.isValid())
            {
                color = MessageColor(userColor);
            }
        }

        builder
            ->emplace<TextElement>(user.userName,
                                   MessageElementFlag::BoldUsername, color,
                                   FontStyle::ChatMediumBold)
            ->setLink({Link::UserInfo, user.userLogin})
            ->setTrailingSpace(false);
        builder
            ->emplace<TextElement>(user.userName,
                                   MessageElementFlag::NonBoldUsername, color)
            ->setLink({Link::UserInfo, user.userLogin})
            ->setTrailingSpace(false);
    }

    builder->message().messageText = text;
    builder->message().searchText = text;
}

MessagePtr TwitchMessageBuilder::buildHypeChatMessage(
    Communi::IrcPrivateMessage *message)
{
    auto levelID = message->tag(u"pinned-chat-paid-level"_s).toString();
    auto currency = message->tag(u"pinned-chat-paid-currency"_s).toString();
    bool okAmount = false;
    auto amount = message->tag(u"pinned-chat-paid-amount"_s).toInt(&okAmount);
    bool okExponent = false;
    auto exponent =
        message->tag(u"pinned-chat-paid-exponent"_s).toInt(&okExponent);
    if (!okAmount || !okExponent || currency.isEmpty())
    {
        return {};
    }
    // additionally, there's `pinned-chat-paid-is-system-message` which isn't used by Chatterino.

    QString subtitle;
    auto levelIt = HYPE_CHAT_PAID_LEVEL.find(levelID);
    if (levelIt != HYPE_CHAT_PAID_LEVEL.end())
    {
        const auto &level = levelIt->second;
        subtitle = u"Level %1 Hype Chat (%2) "_s.arg(level.numeric)
                       .arg(formatTime(level.duration));
    }
    else
    {
        subtitle = u"Hype Chat "_s;
    }

    // actualAmount = amount * 10^(-exponent)
    double actualAmount = std::pow(10.0, double(-exponent)) * double(amount);
    subtitle += QLocale::system().toCurrencyString(actualAmount, currency);

    MessageBuilder builder(systemMessage, parseTagString(subtitle),
                           calculateMessageTime(message).time());
    builder->flags.set(MessageFlag::ElevatedMessage);
    return builder.release();
}

EmotePtr makeAutoModBadge()
{
    return std::make_shared<Emote>(Emote{
        EmoteName{},
        ImageSet{Image::fromResourcePixmap(getResources().twitch.automod)},
        Tooltip{"AutoMod"},
        Url{"https://dashboard.twitch.tv/settings/moderation/automod"}});
}

MessagePtr TwitchMessageBuilder::makeAutomodInfoMessage(
    const AutomodInfoAction &action)
{
    auto builder = MessageBuilder();
    QString text("AutoMod: ");

    builder.emplace<TimestampElement>();
    builder.message().flags.set(MessageFlag::PubSub);

    // AutoMod shield badge
    builder.emplace<BadgeElement>(makeAutoModBadge(),
                                  MessageElementFlag::BadgeChannelAuthority);
    // AutoMod "username"
    builder.emplace<TextElement>("AutoMod:", MessageElementFlag::BoldUsername,
                                 MessageColor(QColor("blue")),
                                 FontStyle::ChatMediumBold);
    builder.emplace<TextElement>(
        "AutoMod:", MessageElementFlag::NonBoldUsername,
        MessageColor(QColor("blue")));
    switch (action.type)
    {
        case AutomodInfoAction::OnHold: {
            QString info("Hey! Your message is being checked "
                         "by mods and has not been sent.");
            text += info;
            builder.emplace<TextElement>(info, MessageElementFlag::Text,
                                         MessageColor::Text);
        }
        break;
        case AutomodInfoAction::Denied: {
            QString info("Mods have removed your message.");
            text += info;
            builder.emplace<TextElement>(info, MessageElementFlag::Text,
                                         MessageColor::Text);
        }
        break;
        case AutomodInfoAction::Approved: {
            QString info("Mods have accepted your message.");
            text += info;
            builder.emplace<TextElement>(info, MessageElementFlag::Text,
                                         MessageColor::Text);
        }
        break;
    }

    builder.message().flags.set(MessageFlag::AutoMod);
    builder.message().messageText = text;
    builder.message().searchText = text;

    auto message = builder.release();

    return message;
}

std::pair<MessagePtr, MessagePtr> TwitchMessageBuilder::makeAutomodMessage(
    const AutomodAction &action, const QString &channelName)
{
    MessageBuilder builder, builder2;

    //
    // Builder for AutoMod message with explanation
    builder.message().loginName = "automod";
    builder.message().channelName = channelName;
    builder.message().flags.set(MessageFlag::PubSub);
    builder.message().flags.set(MessageFlag::Timeout);
    builder.message().flags.set(MessageFlag::AutoMod);
    builder.message().flags.set(MessageFlag::AutoModOffendingMessageHeader);

    // AutoMod shield badge
    builder.emplace<BadgeElement>(makeAutoModBadge(),
                                  MessageElementFlag::BadgeChannelAuthority);
    // AutoMod "username"
    builder.emplace<TextElement>("AutoMod:", MessageElementFlag::BoldUsername,
                                 MessageColor(QColor("blue")),
                                 FontStyle::ChatMediumBold);
    builder.emplace<TextElement>(
        "AutoMod:", MessageElementFlag::NonBoldUsername,
        MessageColor(QColor("blue")));
    // AutoMod header message
    builder.emplace<TextElement>(
        ("Held a message for reason: " + action.reason +
         ". Allow will post it in chat. "),
        MessageElementFlag::Text, MessageColor::Text);
    // Allow link button
    builder
        .emplace<TextElement>("Allow", MessageElementFlag::Text,
                              MessageColor(QColor("green")),
                              FontStyle::ChatMediumBold)
        ->setLink({Link::AutoModAllow, action.msgID});
    // Deny link button
    builder
        .emplace<TextElement>(" Deny", MessageElementFlag::Text,
                              MessageColor(QColor("red")),
                              FontStyle::ChatMediumBold)
        ->setLink({Link::AutoModDeny, action.msgID});
    // ID of message caught by AutoMod
    //    builder.emplace<TextElement>(action.msgID, MessageElementFlag::Text,
    //                                 MessageColor::Text);
    auto text1 =
        QString("AutoMod: Held a message for reason: %1. Allow will post "
                "it in chat. Allow Deny")
            .arg(action.reason);
    builder.message().messageText = text1;
    builder.message().searchText = text1;

    auto message1 = builder.release();

    //
    // Builder for offender's message
    builder2.message().channelName = channelName;
    builder2
        .emplace<TextElement>("#" + channelName,
                              MessageElementFlag::ChannelName,
                              MessageColor::System)
        ->setLink({Link::JumpToChannel, channelName});
    builder2.emplace<TimestampElement>();
    builder2.emplace<TwitchModerationElement>();
    builder2.message().loginName = action.target.login;
    builder2.message().flags.set(MessageFlag::PubSub);
    builder2.message().flags.set(MessageFlag::Timeout);
    builder2.message().flags.set(MessageFlag::AutoMod);
    builder2.message().flags.set(MessageFlag::AutoModOffendingMessage);

    // sender username
    builder2
        .emplace<TextElement>(
            action.target.displayName + ":", MessageElementFlag::BoldUsername,
            MessageColor(action.target.color), FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, action.target.login});
    builder2
        .emplace<TextElement>(action.target.displayName + ":",
                              MessageElementFlag::NonBoldUsername,
                              MessageColor(action.target.color))
        ->setLink({Link::UserInfo, action.target.login});
    // sender's message caught by AutoMod
    builder2.emplace<TextElement>(action.message, MessageElementFlag::Text,
                                  MessageColor::Text);
    auto text2 =
        QString("%1: %2").arg(action.target.displayName, action.message);
    builder2.message().messageText = text2;
    builder2.message().searchText = text2;

    auto message2 = builder2.release();

    // Normally highlights would be checked & triggered during the builder parse steps
    // and when the message is added to the channel
    // We do this a bit weird since the message comes in from PubSub and not the normal message route
    auto [highlighted, highlightResult] = getIApp()->getHighlights()->check(
        {}, {}, action.target.login, action.message, message2->flags);
    if (highlighted)
    {
        SharedMessageBuilder::triggerHighlights(
            channelName, highlightResult.playSound,
            highlightResult.customSoundUrl, highlightResult.alert);
    }

    return std::make_pair(message1, message2);
}

MessagePtr TwitchMessageBuilder::makeLowTrustUpdateMessage(
    const PubSubLowTrustUsersMessage &action)
{
    /**
     * Known issues:
     *  - Non-Twitch badges are not shown
     *  - Non-Twitch emotes are not shown
     */

    MessageBuilder builder;
    builder.emplace<TimestampElement>();
    builder.message().flags.set(MessageFlag::System);
    builder.message().flags.set(MessageFlag::PubSub);
    builder.message().flags.set(MessageFlag::DoNotTriggerNotification);

    builder
        .emplace<TextElement>(action.updatedByUserDisplayName,
                              MessageElementFlag::Username,
                              MessageColor::System, FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, action.updatedByUserLogin});

    assert(action.treatment != PubSubLowTrustUsersMessage::Treatment::INVALID);
    switch (action.treatment)
    {
        case PubSubLowTrustUsersMessage::Treatment::NoTreatment: {
            builder.emplace<TextElement>("removed", MessageElementFlag::Text,
                                         MessageColor::System);
            builder
                .emplace<TextElement>(action.suspiciousUserDisplayName,
                                      MessageElementFlag::Username,
                                      MessageColor::System,
                                      FontStyle::ChatMediumBold)
                ->setLink({Link::UserInfo, action.suspiciousUserLogin});
            builder.emplace<TextElement>("from the suspicious user list.",
                                         MessageElementFlag::Text,
                                         MessageColor::System);
        }
        break;

        case PubSubLowTrustUsersMessage::Treatment::ActiveMonitoring: {
            builder.emplace<TextElement>("added", MessageElementFlag::Text,
                                         MessageColor::System);
            builder
                .emplace<TextElement>(action.suspiciousUserDisplayName,
                                      MessageElementFlag::Username,
                                      MessageColor::System,
                                      FontStyle::ChatMediumBold)
                ->setLink({Link::UserInfo, action.suspiciousUserLogin});
            builder.emplace<TextElement>("as a monitored suspicious chatter.",
                                         MessageElementFlag::Text,
                                         MessageColor::System);
        }
        break;

        case PubSubLowTrustUsersMessage::Treatment::Restricted: {
            builder.emplace<TextElement>("added", MessageElementFlag::Text,
                                         MessageColor::System);
            builder
                .emplace<TextElement>(action.suspiciousUserDisplayName,
                                      MessageElementFlag::Username,
                                      MessageColor::System,
                                      FontStyle::ChatMediumBold)
                ->setLink({Link::UserInfo, action.suspiciousUserLogin});
            builder.emplace<TextElement>("as a restricted suspicious chatter.",
                                         MessageElementFlag::Text,
                                         MessageColor::System);
        }
        break;

        default:
            qCDebug(chatterinoTwitch) << "Unexpected suspicious treatment: "
                                      << action.treatmentString;
            break;
    }

    return builder.release();
}

std::pair<MessagePtr, MessagePtr> TwitchMessageBuilder::makeLowTrustUserMessage(
    const PubSubLowTrustUsersMessage &action, const QString &channelName,
    const TwitchChannel *twitchChannel)
{
    MessageBuilder builder, builder2;

    // Builder for low trust user message with explanation
    builder.message().channelName = channelName;
    builder.message().flags.set(MessageFlag::PubSub);
    builder.message().flags.set(MessageFlag::LowTrustUsers);

    // AutoMod shield badge
    builder.emplace<BadgeElement>(makeAutoModBadge(),
                                  MessageElementFlag::BadgeChannelAuthority);

    // Suspicious user header message
    QString prefix = "Suspicious User:";
    builder.emplace<TextElement>(prefix, MessageElementFlag::Text,
                                 MessageColor(QColor("blue")),
                                 FontStyle::ChatMediumBold);

    QString headerMessage;
    if (action.treatment == PubSubLowTrustUsersMessage::Treatment::Restricted)
    {
        headerMessage = "Restricted";
        builder2.message().flags.set(MessageFlag::RestrictedMessage);
    }
    else
    {
        headerMessage = "Monitored";
        builder2.message().flags.set(MessageFlag::MonitoredMessage);
    }

    if (action.restrictionTypes.has(
            PubSubLowTrustUsersMessage::RestrictionType::ManuallyAdded))
    {
        headerMessage += " by " + action.updatedByUserLogin;
    }

    headerMessage += " at " + action.updatedAt;

    if (action.restrictionTypes.has(
            PubSubLowTrustUsersMessage::RestrictionType::DetectedBanEvader))
    {
        QString evader;
        if (action.evasionEvaluation ==
            PubSubLowTrustUsersMessage::EvasionEvaluation::LikelyEvader)
        {
            evader = "likely";
        }
        else
        {
            evader = "possible";
        }

        headerMessage += ". Detected as " + evader + " ban evader";
    }

    if (action.restrictionTypes.has(
            PubSubLowTrustUsersMessage::RestrictionType::BannedInSharedChannel))
    {
        headerMessage += ". Banned in " +
                         QString::number(action.sharedBanChannelIDs.size()) +
                         " shared channels";
    }

    builder.emplace<TextElement>(headerMessage, MessageElementFlag::Text,
                                 MessageColor::Text);
    builder.message().messageText = prefix + " " + headerMessage;
    builder.message().searchText = prefix + " " + headerMessage;

    auto message1 = builder.release();

    //
    // Builder for offender's message
    builder2.message().channelName = channelName;
    builder2
        .emplace<TextElement>("#" + channelName,
                              MessageElementFlag::ChannelName,
                              MessageColor::System)
        ->setLink({Link::JumpToChannel, channelName});
    builder2.emplace<TimestampElement>();
    builder2.emplace<TwitchModerationElement>();
    builder2.message().loginName = action.suspiciousUserLogin;
    builder2.message().flags.set(MessageFlag::PubSub);
    builder2.message().flags.set(MessageFlag::LowTrustUsers);

    // sender badges
    appendBadges(&builder2, action.senderBadges, {}, twitchChannel);

    // sender username
    builder2
        .emplace<TextElement>(action.suspiciousUserDisplayName + ":",
                              MessageElementFlag::BoldUsername,
                              MessageColor(action.suspiciousUserColor),
                              FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, action.suspiciousUserLogin});
    builder2
        .emplace<TextElement>(action.suspiciousUserDisplayName + ":",
                              MessageElementFlag::NonBoldUsername,
                              MessageColor(action.suspiciousUserColor))
        ->setLink({Link::UserInfo, action.suspiciousUserLogin});

    // sender's message caught by AutoMod
    for (const auto &fragment : action.fragments)
    {
        if (fragment.emoteID.isEmpty())
        {
            builder2.emplace<TextElement>(
                fragment.text, MessageElementFlag::Text, MessageColor::Text);
        }
        else
        {
            const auto emotePtr =
                getIApp()->getEmotes()->getTwitchEmotes()->getOrCreateEmote(
                    EmoteId{fragment.emoteID}, EmoteName{fragment.text});
            builder2.emplace<EmoteElement>(
                emotePtr, MessageElementFlag::TwitchEmote, MessageColor::Text);
        }
    }

    auto text =
        QString("%1: %2").arg(action.suspiciousUserDisplayName, action.text);
    builder2.message().messageText = text;
    builder2.message().searchText = text;

    auto message2 = builder2.release();

    return std::make_pair(message1, message2);
}

void TwitchMessageBuilder::setThread(std::shared_ptr<MessageThread> thread)
{
    this->thread_ = std::move(thread);
}

void TwitchMessageBuilder::setParent(MessagePtr parent)
{
    this->parent_ = std::move(parent);
}

void TwitchMessageBuilder::setMessageOffset(int offset)
{
    this->messageOffset_ = offset;
}

}  // namespace chatterino
