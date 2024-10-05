#include "messages/MessageBuilder.hpp"

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
#include "messages/MessageColor.hpp"
#include "messages/MessageElement.hpp"
#include "messages/MessageThread.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/chatterino/ChatterinoBadges.hpp"
#include "providers/colors/ColorProvider.hpp"
#include "providers/ffz/FfzBadges.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/links/LinkResolver.hpp"
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
#include "singletons/StreamerMode.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/FormatTime.hpp"
#include "util/Helpers.hpp"
#include "util/IrcHelpers.hpp"
#include "util/QStringHash.hpp"
#include "widgets/Window.hpp"

#include <boost/variant.hpp>
#include <QApplication>
#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

#include <chrono>
#include <unordered_set>

using namespace chatterino::literals;

namespace {

using namespace chatterino;
using namespace std::chrono_literals;

const QColor AUTOMOD_USER_COLOR{"blue"};

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

QString formatUpdatedEmoteList(const QString &platform,
                               const std::vector<QString> &emoteNames,
                               bool isAdd, bool isFirstWord)
{
    QString text = "";
    if (isAdd)
    {
        text += isFirstWord ? "Added" : "added";
    }
    else
    {
        text += isFirstWord ? "Removed" : "removed";
    }

    if (emoteNames.size() == 1)
    {
        text += QString(" %1 emote ").arg(platform);
    }
    else
    {
        text += QString(" %1 %2 emotes ").arg(emoteNames.size()).arg(platform);
    }

    auto i = 0;
    for (const auto &emoteName : emoteNames)
    {
        i++;
        if (i > 1)
        {
            text += i == emoteNames.size() ? " and " : ", ";
        }
        text += emoteName;
    }

    text += ".";

    return text;
}

/**
 * Gets the default sound url if the user set one,
 * or the chatterino default ping sound if no url is set.
 */
QUrl getFallbackHighlightSound()
{
    QString path = getSettings()->pathHighlightSound;
    bool fileExists =
        !path.isEmpty() && QFileInfo::exists(path) && QFileInfo(path).isFile();

    if (fileExists)
    {
        return QUrl::fromLocalFile(path);
    }

    return QUrl("qrc:/sounds/ping2.wav");
}

void actuallyTriggerHighlights(const QString &channelName, bool playSound,
                               const std::optional<QUrl> &customSoundUrl,
                               bool windowAlert)
{
    if (getApp()->getStreamerMode()->isEnabled() &&
        getSettings()->streamerModeMuteMentions)
    {
        // We are in streamer mode with muting mention sounds enabled. Do nothing.
        return;
    }

    if (getSettings()->isMutedChannel(channelName))
    {
        // Do nothing. Pings are muted in this channel.
        return;
    }

    const bool hasFocus = (QApplication::focusWidget() != nullptr);
    const bool resolveFocus =
        !hasFocus || getSettings()->highlightAlwaysPlaySound;

    if (playSound && resolveFocus)
    {
        // TODO(C++23): optional or_else
        QUrl soundUrl;
        if (customSoundUrl)
        {
            soundUrl = *customSoundUrl;
        }
        else
        {
            soundUrl = getFallbackHighlightSound();
        }
        getApp()->getSound()->play(soundUrl);
    }

    if (windowAlert)
    {
        getApp()->getWindows()->sendAlert();
    }
}

QString stylizeUsername(const QString &username, const Message &message)
{
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

    if (auto nicknameText = getSettings()->matchNickname(usernameText))
    {
        usernameText = *nicknameText;
    }

    return usernameText;
}

void appendTwitchEmoteOccurrences(const QString &emote,
                                  std::vector<TwitchEmoteOccurrence> &vec,
                                  const std::vector<int> &correctPositions,
                                  const QString &originalMessage,
                                  int messageOffset)
{
    auto *app = getApp();
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
                << "Emote coords" << from << "-" << to << "are out of range ("
                << maxPositions << ")";
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
            getApp()->getTwitchBadges()->badge(badge.key_, badge.value_))
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
        else if (badge.key_ == "vip" && getSettings()->useCustomFfzVipBadges)
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

EmotePtr makeAutoModBadge()
{
    return std::make_shared<Emote>(Emote{
        EmoteName{},
        ImageSet{Image::fromResourcePixmap(getResources().twitch.automod)},
        Tooltip{"AutoMod"},
        Url{"https://dashboard.twitch.tv/settings/moderation/automod"}});
}

}  // namespace

namespace chatterino {

MessagePtr makeSystemMessage(const QString &text)
{
    return MessageBuilder(systemMessage, text).release();
}

MessagePtr makeSystemMessage(const QString &text, const QTime &time)
{
    return MessageBuilder(systemMessage, text, time).release();
}

MessageBuilder::MessageBuilder()
    : message_(std::make_shared<Message>())
    , ircMessage(nullptr)
{
}

MessageBuilder::MessageBuilder(Channel *_channel,
                               const Communi::IrcPrivateMessage *_ircMessage,
                               const MessageParseArgs &_args)
    : twitchChannel(dynamic_cast<TwitchChannel *>(_channel))
    , message_(std::make_shared<Message>())
    , channel(_channel)
    , ircMessage(_ircMessage)
    , args(_args)
    , tags(this->ircMessage->tags())
    , originalMessage_(_ircMessage->content())
    , action_(_ircMessage->isAction())
{
}

MessageBuilder::MessageBuilder(Channel *_channel,
                               const Communi::IrcMessage *_ircMessage,
                               const MessageParseArgs &_args, QString content,
                               bool isAction)
    : twitchChannel(dynamic_cast<TwitchChannel *>(_channel))
    , message_(std::make_shared<Message>())
    , channel(_channel)
    , ircMessage(_ircMessage)
    , args(_args)
    , tags(this->ircMessage->tags())
    , originalMessage_(content)
    , action_(isAction)
{
}

MessageBuilder::MessageBuilder(SystemMessageTag, const QString &text,
                               const QTime &time)
    : MessageBuilder()
{
    this->emplace<TimestampElement>(time);

    // check system message for links
    // (e.g. needed for sub ticket message in sub only mode)
    const QStringList textFragments =
        text.split(QRegularExpression("\\s"), Qt::SkipEmptyParts);
    for (const auto &word : textFragments)
    {
        auto link = linkparser::parse(word);
        if (link)
        {
            this->addLink(*link, word);
            continue;
        }

        this->emplace<TextElement>(word, MessageElementFlag::Text,
                                   MessageColor::System);
    }
    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);
    this->message().messageText = text;
    this->message().searchText = text;
}

MessageBuilder::MessageBuilder(TimeoutMessageTag, const QString &timeoutUser,
                               const QString &sourceUser,
                               const QString &systemMessageText, int times,
                               const QTime &time)
    : MessageBuilder()
{
    QString usernameText = systemMessageText.split(" ").at(0);
    QString remainder = systemMessageText.mid(usernameText.length() + 1);
    bool timeoutUserIsFirst =
        usernameText == "You" || timeoutUser == usernameText;
    QString messageText;

    this->emplace<TimestampElement>(time);
    this->emplaceSystemTextAndUpdate(usernameText, messageText)
        ->setLink(
            {Link::UserInfo, timeoutUserIsFirst ? timeoutUser : sourceUser});

    if (!sourceUser.isEmpty())
    {
        // the second username in the message
        const auto &targetUsername =
            timeoutUserIsFirst ? sourceUser : timeoutUser;
        int userPos = remainder.indexOf(targetUsername);

        QString mid = remainder.mid(0, userPos - 1);
        QString username = remainder.mid(userPos, targetUsername.length());
        remainder = remainder.mid(userPos + targetUsername.length() + 1);

        this->emplaceSystemTextAndUpdate(mid, messageText);
        this->emplaceSystemTextAndUpdate(username, messageText)
            ->setLink({Link::UserInfo, username});
    }

    this->emplaceSystemTextAndUpdate(
        QString("%1 (%2 times)").arg(remainder.trimmed()).arg(times),
        messageText);

    this->message().messageText = messageText;
    this->message().searchText = messageText;
}

MessageBuilder::MessageBuilder(TimeoutMessageTag, const QString &username,
                               const QString &durationInSeconds,
                               bool multipleTimes, const QTime &time)
    : MessageBuilder()
{
    QString fullText;
    QString text;

    this->emplace<TimestampElement>(time);
    this->emplaceSystemTextAndUpdate(username, fullText)
        ->setLink({Link::UserInfo, username});

    if (!durationInSeconds.isEmpty())
    {
        text.append("has been timed out");

        // TODO: Implement who timed the user out

        text.append(" for ");
        bool ok = true;
        int timeoutSeconds = durationInSeconds.toInt(&ok);
        if (ok)
        {
            text.append(formatTime(timeoutSeconds));
        }
    }
    else
    {
        text.append("has been permanently banned");
    }

    text.append(".");

    if (multipleTimes)
    {
        text.append(" (multiple times)");
    }

    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::Timeout);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);
    this->message().timeoutUser = username;

    this->emplaceSystemTextAndUpdate(text, fullText);
    this->message().messageText = fullText;
    this->message().searchText = fullText;
}

MessageBuilder::MessageBuilder(const BanAction &action, uint32_t count)
    : MessageBuilder()
{
    auto current = getApp()->getAccounts()->twitch.getCurrent();

    this->emplace<TimestampElement>();
    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::Timeout);
    this->message().timeoutUser = action.target.login;
    this->message().loginName = action.source.login;
    this->message().count = count;

    QString text;

    if (action.target.id == current->getUserId())
    {
        this->emplaceSystemTextAndUpdate("You", text)
            ->setLink({Link::UserInfo, current->getUserName()});
        this->emplaceSystemTextAndUpdate("were", text);
        if (action.isBan())
        {
            this->emplaceSystemTextAndUpdate("banned", text);
        }
        else
        {
            this->emplaceSystemTextAndUpdate(
                QString("timed out for %1").arg(formatTime(action.duration)),
                text);
        }

        if (!action.source.login.isEmpty())
        {
            this->emplaceSystemTextAndUpdate("by", text);
            this->emplaceSystemTextAndUpdate(
                    action.source.login + (action.reason.isEmpty() ? "." : ":"),
                    text)
                ->setLink({Link::UserInfo, action.source.login});
        }

        if (!action.reason.isEmpty())
        {
            this->emplaceSystemTextAndUpdate(
                QString("\"%1\".").arg(action.reason), text);
        }
    }
    else
    {
        if (action.isBan())
        {
            this->emplaceSystemTextAndUpdate(action.source.login, text)
                ->setLink({Link::UserInfo, action.source.login});
            this->emplaceSystemTextAndUpdate("banned", text);
            if (action.reason.isEmpty())
            {
                this->emplaceSystemTextAndUpdate(action.target.login + ".",
                                                 text)
                    ->setLink({Link::UserInfo, action.target.login});
            }
            else
            {
                this->emplaceSystemTextAndUpdate(action.target.login + ":",
                                                 text)
                    ->setLink({Link::UserInfo, action.target.login});
                this->emplaceSystemTextAndUpdate(
                    QString("\"%1\".").arg(action.reason), text);
            }
        }
        else
        {
            this->emplaceSystemTextAndUpdate(action.source.login, text)
                ->setLink({Link::UserInfo, action.source.login});
            this->emplaceSystemTextAndUpdate("timed out", text);
            this->emplaceSystemTextAndUpdate(action.target.login, text)
                ->setLink({Link::UserInfo, action.target.login});
            if (action.reason.isEmpty())
            {
                this->emplaceSystemTextAndUpdate(
                    QString("for %1.").arg(formatTime(action.duration)), text);
            }
            else
            {
                this->emplaceSystemTextAndUpdate(
                    QString("for %1: \"%2\".")
                        .arg(formatTime(action.duration))
                        .arg(action.reason),
                    text);
            }

            if (count > 1)
            {
                this->emplaceSystemTextAndUpdate(
                    QString("(%1 times)").arg(count), text);
            }
        }
    }

    this->message().messageText = text;
    this->message().searchText = text;
}

MessageBuilder::MessageBuilder(const UnbanAction &action)
    : MessageBuilder()
{
    this->emplace<TimestampElement>();
    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::Untimeout);

    this->message().timeoutUser = action.target.login;

    QString text;

    this->emplaceSystemTextAndUpdate(action.source.login, text)
        ->setLink({Link::UserInfo, action.source.login});
    this->emplaceSystemTextAndUpdate(
        action.wasBan() ? "unbanned" : "untimedout", text);
    this->emplaceSystemTextAndUpdate(action.target.login + ".", text)
        ->setLink({Link::UserInfo, action.target.login});

    this->message().messageText = text;
    this->message().searchText = text;
}

MessageBuilder::MessageBuilder(const WarnAction &action)
    : MessageBuilder()
{
    this->emplace<TimestampElement>();
    this->message().flags.set(MessageFlag::System);

    QString text;

    // TODO: Use MentionElement here, once WarnAction includes username/displayname
    this->emplaceSystemTextAndUpdate("A moderator", text)
        ->setLink({Link::UserInfo, "id:" + action.source.id});
    this->emplaceSystemTextAndUpdate("warned", text);
    this->emplaceSystemTextAndUpdate(
            action.target.login + (action.reasons.isEmpty() ? "." : ":"), text)
        ->setLink({Link::UserInfo, action.target.login});

    if (!action.reasons.isEmpty())
    {
        this->emplaceSystemTextAndUpdate(action.reasons.join(", "), text);
    }

    this->message().messageText = text;
    this->message().searchText = text;
}

MessageBuilder::MessageBuilder(const RaidAction &action)
    : MessageBuilder()
{
    this->emplace<TimestampElement>();
    this->message().flags.set(MessageFlag::System);

    QString text;

    this->emplaceSystemTextAndUpdate(action.source.login, text)
        ->setLink({Link::UserInfo, "id:" + action.source.id});
    this->emplaceSystemTextAndUpdate("initiated a raid to", text);
    this->emplaceSystemTextAndUpdate(action.target + ".", text)
        ->setLink({Link::UserInfo, action.target});

    this->message().messageText = text;
    this->message().searchText = text;
}

MessageBuilder::MessageBuilder(const UnraidAction &action)
    : MessageBuilder()
{
    this->emplace<TimestampElement>();
    this->message().flags.set(MessageFlag::System);

    QString text;

    this->emplaceSystemTextAndUpdate(action.source.login, text)
        ->setLink({Link::UserInfo, "id:" + action.source.id});
    this->emplaceSystemTextAndUpdate("canceled the raid.", text);

    this->message().messageText = text;
    this->message().searchText = text;
}

MessageBuilder::MessageBuilder(const AutomodUserAction &action)
    : MessageBuilder()
{
    this->emplace<TimestampElement>();
    this->message().flags.set(MessageFlag::System);

    QString text;
    switch (action.type)
    {
        case AutomodUserAction::AddPermitted: {
            text = QString("%1 added \"%2\" as a permitted term on AutoMod.")
                       .arg(action.source.login, action.message);
        }
        break;

        case AutomodUserAction::AddBlocked: {
            text = QString("%1 added \"%2\" as a blocked term on AutoMod.")
                       .arg(action.source.login, action.message);
        }
        break;

        case AutomodUserAction::RemovePermitted: {
            text = QString("%1 removed \"%2\" as a permitted term on AutoMod.")
                       .arg(action.source.login, action.message);
        }
        break;

        case AutomodUserAction::RemoveBlocked: {
            text = QString("%1 removed \"%2\" as a blocked term on AutoMod.")
                       .arg(action.source.login, action.message);
        }
        break;

        case AutomodUserAction::Properties: {
            text = QString("%1 modified the AutoMod properties.")
                       .arg(action.source.login);
        }
        break;
    }
    this->message().messageText = text;
    this->message().searchText = text;

    this->emplace<TextElement>(text, MessageElementFlag::Text,
                               MessageColor::System);
}

MessageBuilder::MessageBuilder(LiveUpdatesAddEmoteMessageTag /*unused*/,
                               const QString &platform, const QString &actor,
                               const std::vector<QString> &emoteNames)
    : MessageBuilder()
{
    auto text =
        formatUpdatedEmoteList(platform, emoteNames, true, actor.isEmpty());

    this->emplace<TimestampElement>();
    if (!actor.isEmpty())
    {
        this->emplace<TextElement>(actor, MessageElementFlag::Username,
                                   MessageColor::System)
            ->setLink({Link::UserInfo, actor});
    }
    this->emplace<TextElement>(text, MessageElementFlag::Text,
                               MessageColor::System);

    QString finalText;
    if (actor.isEmpty())
    {
        finalText = text;
    }
    else
    {
        finalText = QString("%1 %2").arg(actor, text);
    }

    this->message().loginName = actor;
    this->message().messageText = finalText;
    this->message().searchText = finalText;

    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::LiveUpdatesAdd);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);
}

MessageBuilder::MessageBuilder(LiveUpdatesRemoveEmoteMessageTag /*unused*/,
                               const QString &platform, const QString &actor,
                               const std::vector<QString> &emoteNames)
    : MessageBuilder()
{
    auto text =
        formatUpdatedEmoteList(platform, emoteNames, false, actor.isEmpty());

    this->emplace<TimestampElement>();
    if (!actor.isEmpty())
    {
        this->emplace<TextElement>(actor, MessageElementFlag::Username,
                                   MessageColor::System)
            ->setLink({Link::UserInfo, actor});
    }
    this->emplace<TextElement>(text, MessageElementFlag::Text,
                               MessageColor::System);

    QString finalText;
    if (actor.isEmpty())
    {
        finalText = text;
    }
    else
    {
        finalText = QString("%1 %2").arg(actor, text);
    }

    this->message().loginName = actor;
    this->message().messageText = finalText;
    this->message().searchText = finalText;

    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::LiveUpdatesRemove);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);
}

MessageBuilder::MessageBuilder(LiveUpdatesUpdateEmoteMessageTag /*unused*/,
                               const QString &platform, const QString &actor,
                               const QString &emoteName,
                               const QString &oldEmoteName)
    : MessageBuilder()
{
    QString text;
    if (actor.isEmpty())
    {
        text = "Renamed";
    }
    else
    {
        text = "renamed";
    }
    text +=
        QString(" %1 emote %2 to %3.").arg(platform, oldEmoteName, emoteName);

    this->emplace<TimestampElement>();
    if (!actor.isEmpty())
    {
        this->emplace<TextElement>(actor, MessageElementFlag::Username,
                                   MessageColor::System)
            ->setLink({Link::UserInfo, actor});
    }
    this->emplace<TextElement>(text, MessageElementFlag::Text,
                               MessageColor::System);

    QString finalText;
    if (actor.isEmpty())
    {
        finalText = text;
    }
    else
    {
        finalText = QString("%1 %2").arg(actor, text);
    }

    this->message().loginName = actor;
    this->message().messageText = finalText;
    this->message().searchText = finalText;

    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::LiveUpdatesUpdate);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);
}

MessageBuilder::MessageBuilder(LiveUpdatesUpdateEmoteSetMessageTag /*unused*/,
                               const QString &platform, const QString &actor,
                               const QString &emoteSetName)
    : MessageBuilder()
{
    auto text = QString("switched the active %1 Emote Set to \"%2\".")
                    .arg(platform, emoteSetName);

    this->emplace<TimestampElement>();
    this->emplace<TextElement>(actor, MessageElementFlag::Username,
                               MessageColor::System)
        ->setLink({Link::UserInfo, actor});
    this->emplace<TextElement>(text, MessageElementFlag::Text,
                               MessageColor::System);

    auto finalText = QString("%1 %2").arg(actor, text);

    this->message().loginName = actor;
    this->message().messageText = finalText;
    this->message().searchText = finalText;

    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::LiveUpdatesUpdate);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);
}

MessageBuilder::MessageBuilder(ImageUploaderResultTag /*unused*/,
                               const QString &imageLink,
                               const QString &deletionLink,
                               size_t imagesStillQueued, size_t secondsLeft)
    : MessageBuilder()
{
    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);

    this->emplace<TimestampElement>();

    using MEF = MessageElementFlag;
    auto addText = [this](QString text,
                          MessageColor color =
                              MessageColor::System) -> TextElement * {
        this->message().searchText += text;
        this->message().messageText += text;
        return this->emplace<TextElement>(text, MEF::Text, color);
    };

    addText("Your image has been uploaded to");

    // ASSUMPTION: the user gave this uploader configuration to the program
    // therefore they trust that the host is not wrong/malicious. This doesn't obey getSettings()->lowercaseDomains.
    // This also ensures that the LinkResolver doesn't get these links.
    addText(imageLink, MessageColor::Link)
        ->setLink({Link::Url, imageLink})
        ->setTrailingSpace(!deletionLink.isEmpty());

    if (!deletionLink.isEmpty())
    {
        addText("(Deletion link:");
        addText(deletionLink, MessageColor::Link)
            ->setLink({Link::Url, deletionLink})
            ->setTrailingSpace(false);
        addText(")")->setTrailingSpace(false);
    }
    addText(".");

    if (imagesStillQueued == 0)
    {
        return;
    }

    addText(QString("%1 left. Please wait until all of them are uploaded. "
                    "About %2 seconds left.")
                .arg(imagesStillQueued)
                .arg(secondsLeft));
}

Message *MessageBuilder::operator->()
{
    return this->message_.get();
}

Message &MessageBuilder::message()
{
    return *this->message_;
}

MessagePtr MessageBuilder::release()
{
    std::shared_ptr<Message> ptr;
    this->message_.swap(ptr);
    return ptr;
}

std::weak_ptr<Message> MessageBuilder::weakOf()
{
    return this->message_;
}

void MessageBuilder::append(std::unique_ptr<MessageElement> element)
{
    this->message().elements.push_back(std::move(element));
}

void MessageBuilder::addLink(const linkparser::Parsed &parsedLink,
                             const QString &source)
{
    QString lowercaseLinkString;
    QString origLink = parsedLink.link.toString();
    QString fullUrl;

    if (parsedLink.protocol.isNull())
    {
        fullUrl = QStringLiteral("http://") + origLink;
    }
    else
    {
        lowercaseLinkString += parsedLink.protocol;
        fullUrl = origLink;
    }

    lowercaseLinkString += parsedLink.host.toString().toLower();
    lowercaseLinkString += parsedLink.rest;

    auto textColor = MessageColor(MessageColor::Link);

    if (parsedLink.hasPrefix(source))
    {
        this->emplace<TextElement>(parsedLink.prefix(source).toString(),
                                   MessageElementFlag::Text, this->textColor_)
            ->setTrailingSpace(false);
    }
    auto *el = this->emplace<LinkElement>(
        LinkElement::Parsed{.lowercase = lowercaseLinkString,
                            .original = origLink},
        fullUrl, MessageElementFlag::Text, textColor);
    if (parsedLink.hasSuffix(source))
    {
        el->setTrailingSpace(false);
        this->emplace<TextElement>(parsedLink.suffix(source).toString(),
                                   MessageElementFlag::Text, this->textColor_);
    }

    getApp()->getLinkResolver()->resolve(el->linkInfo());
}

bool MessageBuilder::isIgnored() const
{
    return isIgnoredMessage({
        /*.message = */ this->originalMessage_,
        /*.twitchUserID = */ this->tags.value("user-id").toString(),
        /*.isMod = */ this->channel->isMod(),
        /*.isBroadcaster = */ this->channel->isBroadcaster(),
    });
}

bool MessageBuilder::isIgnoredReply() const
{
    return isIgnoredMessage({
        /*.message = */ this->originalMessage_,
        /*.twitchUserID = */
        this->tags.value("reply-parent-user-id").toString(),
        /*.isMod = */ this->channel->isMod(),
        /*.isBroadcaster = */ this->channel->isBroadcaster(),
    });
}

void MessageBuilder::triggerHighlights()
{
    if (this->historicalMessage_)
    {
        // Do nothing. Highlights should not be triggered on historical messages.
        return;
    }

    actuallyTriggerHighlights(this->channel->getName(), this->highlightSound_,
                              this->highlightSoundCustomUrl_,
                              this->highlightAlert_);
}

MessagePtr MessageBuilder::build()
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
        assert(this->twitchChannel != nullptr);
        const auto &reward = this->twitchChannel->channelPointReward(
            this->args.channelPointRewardId);
        if (reward)
        {
            this->appendChannelPointRewardMessage(
                *reward, this->channel->isMod(),
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
    auto twitchEmotes = MessageBuilder::parseTwitchEmotes(
        this->tags, this->originalMessage_, this->messageOffset_);

    // This runs through all ignored phrases and runs its replacements on this->originalMessage_
    MessageBuilder::processIgnorePhrases(
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

    QString stylizedUsername = stylizeUsername(this->userName, this->message());

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

void MessageBuilder::setThread(std::shared_ptr<MessageThread> thread)
{
    this->thread_ = std::move(thread);
}

void MessageBuilder::setParent(MessagePtr parent)
{
    this->parent_ = std::move(parent);
}

void MessageBuilder::setMessageOffset(int offset)
{
    this->messageOffset_ = offset;
}

void MessageBuilder::appendChannelPointRewardMessage(
    const ChannelPointReward &reward, bool isMod, bool isBroadcaster)
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

    this->emplace<TimestampElement>();
    QString redeemed = "Redeemed";
    QStringList textList;
    if (!reward.isUserInputRequired)
    {
        this->emplace<TextElement>(
                reward.user.login, MessageElementFlag::ChannelPointReward,
                MessageColor::Text, FontStyle::ChatMediumBold)
            ->setLink({Link::UserInfo, reward.user.login});
        redeemed = "redeemed";
        textList.append(reward.user.login);
    }
    this->emplace<TextElement>(redeemed,
                               MessageElementFlag::ChannelPointReward);
    if (reward.id == "CELEBRATION")
    {
        const auto emotePtr =
            getApp()->getEmotes()->getTwitchEmotes()->getOrCreateEmote(
                EmoteId{reward.emoteId}, EmoteName{reward.emoteName});
        this->emplace<EmoteElement>(emotePtr,
                                    MessageElementFlag::ChannelPointReward,
                                    MessageColor::Text);
    }
    this->emplace<TextElement>(reward.title,
                               MessageElementFlag::ChannelPointReward,
                               MessageColor::Text, FontStyle::ChatMediumBold);
    this->emplace<ScalingImageElement>(
        reward.image, MessageElementFlag::ChannelPointRewardImage);
    this->emplace<TextElement>(QString::number(reward.cost),
                               MessageElementFlag::ChannelPointReward,
                               MessageColor::Text, FontStyle::ChatMediumBold);
    if (reward.isBits)
    {
        this->emplace<TextElement>(
            "bits", MessageElementFlag::ChannelPointReward, MessageColor::Text,
            FontStyle::ChatMediumBold);
    }
    if (reward.isUserInputRequired)
    {
        this->emplace<LinebreakElement>(MessageElementFlag::ChannelPointReward);
    }

    this->message().flags.set(MessageFlag::RedeemedChannelPointReward);

    textList.append({redeemed, reward.title, QString::number(reward.cost)});
    this->message().messageText = textList.join(" ");
    this->message().searchText = textList.join(" ");
    this->message().loginName = reward.user.login;

    this->message().reward = std::make_shared<ChannelPointReward>(reward);
}

MessagePtr MessageBuilder::makeChannelPointRewardMessage(
    const ChannelPointReward &reward, bool isMod, bool isBroadcaster)
{
    MessageBuilder builder;

    builder.appendChannelPointRewardMessage(reward, isMod, isBroadcaster);

    return builder.release();
}

MessagePtr MessageBuilder::makeLiveMessage(const QString &channelName,
                                           const QString &channelID,
                                           MessageFlags extraFlags)
{
    MessageBuilder builder;

    builder.emplace<TimestampElement>();
    builder
        .emplace<TextElement>(channelName, MessageElementFlag::Username,
                              MessageColor::Text, FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, channelName});
    builder.emplace<TextElement>("is live!", MessageElementFlag::Text,
                                 MessageColor::Text);
    auto text = QString("%1 is live!").arg(channelName);
    builder.message().messageText = text;
    builder.message().searchText = text;
    builder.message().id = channelID;

    if (!extraFlags.isEmpty())
    {
        builder.message().flags.set(extraFlags);
    }

    return builder.release();
}

MessagePtr MessageBuilder::makeOfflineSystemMessage(const QString &channelName,
                                                    const QString &channelID)
{
    MessageBuilder builder;
    builder.emplace<TimestampElement>();
    builder.message().flags.set(MessageFlag::System);
    builder.message().flags.set(MessageFlag::DoNotTriggerNotification);
    builder
        .emplace<TextElement>(channelName, MessageElementFlag::Username,
                              MessageColor::System, FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, channelName});
    builder.emplace<TextElement>("is now offline.", MessageElementFlag::Text,
                                 MessageColor::System);
    auto text = QString("%1 is now offline.").arg(channelName);
    builder.message().messageText = text;
    builder.message().searchText = text;
    builder.message().id = channelID;

    return builder.release();
}

MessagePtr MessageBuilder::makeHostingSystemMessage(const QString &channelName,
                                                    bool hostOn)
{
    MessageBuilder builder;
    QString text;
    builder.emplace<TimestampElement>();
    builder.message().flags.set(MessageFlag::System);
    builder.message().flags.set(MessageFlag::DoNotTriggerNotification);
    if (hostOn)
    {
        builder.emplace<TextElement>("Now hosting", MessageElementFlag::Text,
                                     MessageColor::System);
        builder
            .emplace<TextElement>(
                channelName + ".", MessageElementFlag::Username,
                MessageColor::System, FontStyle::ChatMediumBold)
            ->setLink({Link::UserInfo, channelName});
        text = QString("Now hosting %1.").arg(channelName);
    }
    else
    {
        builder
            .emplace<TextElement>(channelName, MessageElementFlag::Username,
                                  MessageColor::System,
                                  FontStyle::ChatMediumBold)
            ->setLink({Link::UserInfo, channelName});
        builder.emplace<TextElement>("has gone offline. Exiting host mode.",
                                     MessageElementFlag::Text,
                                     MessageColor::System);
        text =
            QString("%1 has gone offline. Exiting host mode.").arg(channelName);
    }
    builder.message().messageText = text;
    builder.message().searchText = text;
    return builder.release();
}

MessagePtr MessageBuilder::makeDeletionMessageFromIRC(
    const MessagePtr &originalMessage)
{
    MessageBuilder builder;

    builder.emplace<TimestampElement>();
    builder.message().flags.set(MessageFlag::System);
    builder.message().flags.set(MessageFlag::DoNotTriggerNotification);
    builder.message().flags.set(MessageFlag::Timeout);
    // TODO(mm2pl): If or when jumping to a single message gets implemented a link,
    // add a link to the originalMessage
    builder.emplace<TextElement>("A message from", MessageElementFlag::Text,
                                 MessageColor::System);
    builder
        .emplace<TextElement>(originalMessage->displayName,
                              MessageElementFlag::Username,
                              MessageColor::System, FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, originalMessage->loginName});
    builder.emplace<TextElement>("was deleted:", MessageElementFlag::Text,
                                 MessageColor::System);
    if (originalMessage->messageText.length() > 50)
    {
        builder
            .emplace<TextElement>(originalMessage->messageText.left(50) + "…",
                                  MessageElementFlag::Text, MessageColor::Text)
            ->setLink({Link::JumpToMessage, originalMessage->id});
    }
    else
    {
        builder
            .emplace<TextElement>(originalMessage->messageText,
                                  MessageElementFlag::Text, MessageColor::Text)
            ->setLink({Link::JumpToMessage, originalMessage->id});
    }
    builder.message().timeoutUser = "msg:" + originalMessage->id;

    return builder.release();
}

MessagePtr MessageBuilder::makeDeletionMessageFromPubSub(
    const DeleteAction &action)
{
    MessageBuilder builder;

    builder.emplace<TimestampElement>();
    builder.message().flags.set(MessageFlag::System);
    builder.message().flags.set(MessageFlag::DoNotTriggerNotification);
    builder.message().flags.set(MessageFlag::Timeout);

    builder
        .emplace<TextElement>(action.source.login, MessageElementFlag::Username,
                              MessageColor::System, FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, action.source.login});
    // TODO(mm2pl): If or when jumping to a single message gets implemented a link,
    // add a link to the originalMessage
    builder.emplace<TextElement>(
        "deleted message from", MessageElementFlag::Text, MessageColor::System);
    builder
        .emplace<TextElement>(action.target.login, MessageElementFlag::Username,
                              MessageColor::System, FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, action.target.login});
    builder.emplace<TextElement>("saying:", MessageElementFlag::Text,
                                 MessageColor::System);
    if (action.messageText.length() > 50)
    {
        builder
            .emplace<TextElement>(action.messageText.left(50) + "…",
                                  MessageElementFlag::Text, MessageColor::Text)
            ->setLink({Link::JumpToMessage, action.messageId});
    }
    else
    {
        builder
            .emplace<TextElement>(action.messageText, MessageElementFlag::Text,
                                  MessageColor::Text)
            ->setLink({Link::JumpToMessage, action.messageId});
    }
    builder.message().timeoutUser = "msg:" + action.messageId;
    builder.message().flags.set(MessageFlag::PubSub);

    return builder.release();
}

MessagePtr MessageBuilder::makeListOfUsersMessage(QString prefix,
                                                  QStringList users,
                                                  Channel *channel,
                                                  MessageFlags extraFlags)
{
    MessageBuilder builder;

    QString text = prefix + users.join(", ");

    builder.message().messageText = text;
    builder.message().searchText = text;

    builder.emplace<TimestampElement>();
    builder.message().flags.set(MessageFlag::System);
    builder.message().flags.set(MessageFlag::DoNotTriggerNotification);
    builder.emplace<TextElement>(prefix, MessageElementFlag::Text,
                                 MessageColor::System);
    bool isFirst = true;
    auto *tc = dynamic_cast<TwitchChannel *>(channel);
    for (const QString &username : users)
    {
        if (!isFirst)
        {
            // this is used to add the ", " after each but the last entry
            builder.emplace<TextElement>(",", MessageElementFlag::Text,
                                         MessageColor::System);
        }
        isFirst = false;

        MessageColor color = MessageColor::System;

        if (tc)
        {
            if (auto userColor = tc->getUserColor(username);
                userColor.isValid())
            {
                color = MessageColor(userColor);
            }
        }

        // TODO: Ensure we make use of display name / username(login name) correctly here
        builder
            .emplace<MentionElement>(username, username, MessageColor::System,
                                     color)
            ->setTrailingSpace(false);
    }

    if (!extraFlags.isEmpty())
    {
        builder.message().flags.set(extraFlags);
    }

    return builder.release();
}

MessagePtr MessageBuilder::makeListOfUsersMessage(
    QString prefix, const std::vector<HelixModerator> &users, Channel *channel,
    MessageFlags extraFlags)
{
    MessageBuilder builder;

    QString text = prefix;

    builder.emplace<TimestampElement>();
    builder.message().flags.set(MessageFlag::System);
    builder.message().flags.set(MessageFlag::DoNotTriggerNotification);
    builder.emplace<TextElement>(prefix, MessageElementFlag::Text,
                                 MessageColor::System);
    bool isFirst = true;
    auto *tc = dynamic_cast<TwitchChannel *>(channel);
    for (const auto &user : users)
    {
        if (!isFirst)
        {
            // this is used to add the ", " after each but the last entry
            builder.emplace<TextElement>(",", MessageElementFlag::Text,
                                         MessageColor::System);
            text += QString(", %1").arg(user.userName);
        }
        else
        {
            text += user.userName;
        }
        isFirst = false;

        MessageColor color = MessageColor::System;

        if (tc)
        {
            if (auto userColor = tc->getUserColor(user.userLogin);
                userColor.isValid())
            {
                color = MessageColor(userColor);
            }
        }

        builder
            .emplace<MentionElement>(user.userName, user.userLogin,
                                     MessageColor::System, color)
            ->setTrailingSpace(false);
    }

    builder.message().messageText = text;
    builder.message().searchText = text;

    if (!extraFlags.isEmpty())
    {
        builder.message().flags.set(extraFlags);
    }

    return builder.release();
}

MessagePtr MessageBuilder::buildHypeChatMessage(
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

std::pair<MessagePtr, MessagePtr> MessageBuilder::makeAutomodMessage(
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
    builder2.emplace<TextElement>("AutoMod:", MessageElementFlag::Text,
                                  AUTOMOD_USER_COLOR,
                                  FontStyle::ChatMediumBold);
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
    builder2.emplace<MentionElement>(action.target.displayName + ":",
                                     action.target.login, MessageColor::Text,
                                     action.target.color);
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
    auto [highlighted, highlightResult] = getApp()->getHighlights()->check(
        {}, {}, action.target.login, action.message, message2->flags);
    if (highlighted)
    {
        actuallyTriggerHighlights(channelName, highlightResult.playSound,
                                  highlightResult.customSoundUrl,
                                  highlightResult.alert);
    }

    return std::make_pair(message1, message2);
}

MessagePtr MessageBuilder::makeAutomodInfoMessage(
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
    builder.emplace<TextElement>("AutoMod:", MessageElementFlag::Text,
                                 AUTOMOD_USER_COLOR, FontStyle::ChatMediumBold);
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

std::pair<MessagePtr, MessagePtr> MessageBuilder::makeLowTrustUserMessage(
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
    builder2.emplace<MentionElement>(
        action.suspiciousUserDisplayName + ":", action.suspiciousUserLogin,
        MessageColor::Text, action.suspiciousUserColor);

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
                getApp()->getEmotes()->getTwitchEmotes()->getOrCreateEmote(
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

MessagePtr MessageBuilder::makeLowTrustUpdateMessage(
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

std::unordered_map<QString, QString> MessageBuilder::parseBadgeInfoTag(
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
        infoMap.emplace(slashKeyValue(badge));
    }

    return infoMap;
}

std::vector<Badge> MessageBuilder::parseBadgeTag(const QVariantMap &tags)
{
    std::vector<Badge> b;

    auto badgesIt = tags.constFind("badges");
    if (badgesIt == tags.end())
    {
        return b;
    }

    auto badges = badgesIt.value().toString().split(',', Qt::SkipEmptyParts);

    for (const QString &badge : badges)
    {
        if (!badge.contains('/'))
        {
            continue;
        }

        auto pair = slashKeyValue(badge);
        b.emplace_back(Badge{pair.first, pair.second});
    }

    return b;
}

std::vector<TwitchEmoteOccurrence> MessageBuilder::parseTwitchEmotes(
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

void MessageBuilder::processIgnorePhrases(
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

        auto midExtendedRef =
            QStringView{originalMessage}.mid(wordStart, wordEnd - wordStart);

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

void MessageBuilder::addTextOrEmoji(EmotePtr emote)
{
    this->emplace<EmoteElement>(emote, MessageElementFlag::EmojiAll);
}

void MessageBuilder::addTextOrEmoji(const QString &string_)
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
    auto link = linkparser::parse(string);
    auto textColor = this->textColor_;

    if (link)
    {
        this->addLink(*link, string);
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

            if (this->twitchChannel != nullptr)
            {
                if (auto userColor =
                        this->twitchChannel->getUserColor(username);
                    userColor.isValid())
                {
                    textColor = userColor;
                }
            }

            auto prefixedUsername = '@' + username;
            auto remainder = string.remove(prefixedUsername);
            this->emplace<MentionElement>(prefixedUsername, username,
                                          originalTextColor, textColor)
                ->setTrailingSpace(remainder.isEmpty());

            if (!remainder.isEmpty())
            {
                this->emplace<TextElement>(remainder, MessageElementFlag::Text,
                                           originalTextColor);
            }

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

            if (auto userColor = this->twitchChannel->getUserColor(username);
                userColor.isValid())
            {
                textColor = userColor;
            }

            auto remainder = string.remove(username);
            this->emplace<MentionElement>(username, username, originalTextColor,
                                          textColor)
                ->setTrailingSpace(remainder.isEmpty());

            if (!remainder.isEmpty())
            {
                this->emplace<TextElement>(remainder, MessageElementFlag::Text,
                                           originalTextColor);
            }

            return;
        }
    }

    this->emplace<TextElement>(string, MessageElementFlag::Text, textColor);
}

bool MessageBuilder::isEmpty() const
{
    return this->message_->elements.empty();
}

MessageElement &MessageBuilder::back()
{
    assert(!this->isEmpty());
    return *this->message().elements.back();
}

std::unique_ptr<MessageElement> MessageBuilder::releaseBack()
{
    assert(!this->isEmpty());

    auto ptr = std::move(this->message().elements.back());
    this->message().elements.pop_back();
    return ptr;
}

TextElement *MessageBuilder::emplaceSystemTextAndUpdate(const QString &text,
                                                        QString &toUpdate)
{
    toUpdate.append(text + " ");
    return this->emplace<TextElement>(text, MessageElementFlag::Text,
                                      MessageColor::System);
}

void MessageBuilder::parse()
{
    this->parseUsernameColor();

    if (this->action_)
    {
        this->textColor_ = this->usernameColor_;
        this->message().flags.set(MessageFlag::Action);
    }

    this->parseUsername();

    this->message().flags.set(MessageFlag::Collapsed);
}

void MessageBuilder::parseUsernameColor()
{
    const auto *userData = getApp()->getUserData();
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

void MessageBuilder::parseUsername()
{
    // username
    this->userName = this->ircMessage->nick();

    this->message().loginName = this->userName;

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
    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (this->ircMessage->nick() == currentUser->getUserName())
    {
        currentUser->setColor(this->usernameColor_);
    }
}

void MessageBuilder::parseMessageID()
{
    auto iterator = this->tags.find("id");

    if (iterator != this->tags.end())
    {
        this->message().id = iterator.value().toString();
    }
}

void MessageBuilder::parseRoomID()
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

        if (auto it = this->tags.find("source-room-id"); it != this->tags.end())
        {
            auto sourceRoom = it.value().toString();
            if (this->roomID_ != sourceRoom)
            {
                this->message().flags.set(MessageFlag::SharedMessage);

                auto sourceChan =
                    getApp()->getTwitch()->getChannelOrEmptyByID(sourceRoom);
                if (sourceChan)
                {
                    this->sourceChannel =
                        dynamic_cast<TwitchChannel *>(sourceChan.get());
                    // avoid duplicate pings
                    this->message().flags.set(
                        MessageFlag::DoNotTriggerNotification);
                }
            }
        }
    }
}

void MessageBuilder::parseThread()
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

        QString usernameText =
            stylizeUsername(threadRoot->loginName, *threadRoot);

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

void MessageBuilder::parseHighlights()
{
    if (getSettings()->isBlacklistedUser(this->message().loginName))
    {
        // Do nothing. We ignore highlights from this user.
        return;
    }

    auto badges = parseBadgeTag(this->tags);
    auto [highlighted, highlightResult] = getApp()->getHighlights()->check(
        this->args, badges, this->message().loginName, this->originalMessage_,
        this->message().flags);

    if (!highlighted)
    {
        return;
    }

    // This message triggered one or more highlights, act upon the highlight result

    this->message().flags.set(MessageFlag::Highlighted);

    this->highlightAlert_ = highlightResult.alert;

    this->highlightSound_ = highlightResult.playSound;
    this->highlightSoundCustomUrl_ = highlightResult.customSoundUrl;

    this->message().highlightColor = highlightResult.color;

    if (highlightResult.showInMentions)
    {
        this->message().flags.set(MessageFlag::ShowInMentions);
    }
}

void MessageBuilder::appendChannelName()
{
    QString channelName("#" + this->channel->getName());
    Link link(Link::JumpToChannel, this->channel->getName());

    this->emplace<TextElement>(channelName, MessageElementFlag::ChannelName,
                               MessageColor::System)
        ->setLink(link);
}

void MessageBuilder::appendUsername()
{
    auto *app = getApp();

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

const TwitchChannel *MessageBuilder::getSourceChannel() const
{
    if (this->sourceChannel != nullptr)
    {
        return this->sourceChannel;
    }

    return this->twitchChannel;
}

std::tuple<std::optional<EmotePtr>, MessageElementFlags, bool>
    MessageBuilder::parseEmote(const EmoteName &name) const
{
    // Emote order:
    //  - FrankerFaceZ Channel
    //  - BetterTTV Channel
    //  - 7TV Channel
    //  - FrankerFaceZ Global
    //  - BetterTTV Global
    //  - 7TV Global

    const auto *globalFfzEmotes = getApp()->getFfzEmotes();
    const auto *globalBttvEmotes = getApp()->getBttvEmotes();
    const auto *globalSeventvEmotes = getApp()->getSeventvEmotes();

    const auto *sourceChannel = this->getSourceChannel();

    std::optional<EmotePtr> emote{};

    if (sourceChannel != nullptr)
    {
        // Check for channel emotes

        emote = sourceChannel->ffzEmote(name);
        if (emote)
        {
            return {
                emote,
                MessageElementFlag::FfzEmote,
                false,
            };
        }

        emote = sourceChannel->bttvEmote(name);
        if (emote)
        {
            return {
                emote,
                MessageElementFlag::BttvEmote,
                false,
            };
        }

        emote = sourceChannel->seventvEmote(name);
        if (emote)
        {
            return {
                emote,
                MessageElementFlag::SevenTVEmote,
                emote.value()->zeroWidth,
            };
        }
    }

    // Check for global emotes

    emote = globalFfzEmotes->emote(name);
    if (emote)
    {
        return {
            emote,
            MessageElementFlag::FfzEmote,
            false,
        };
    }

    emote = globalBttvEmotes->emote(name);
    if (emote)
    {
        return {
            emote,
            MessageElementFlag::BttvEmote,
            zeroWidthEmotes.contains(name.string),
        };
    }

    emote = globalSeventvEmotes->globalEmote(name);
    if (emote)
    {
        return {
            emote,
            MessageElementFlag::SevenTVEmote,
            emote.value()->zeroWidth,
        };
    }

    return {
        {},
        {},
        false,
    };
}

Outcome MessageBuilder::tryAppendEmote(const EmoteName &name)
{
    const auto [emote, flags, zeroWidth] = this->parseEmote(name);

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

void MessageBuilder::addWords(
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
                 getApp()->getEmotes()->getEmojis()->parse(preText))
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
        for (auto &variant : getApp()->getEmotes()->getEmojis()->parse(word))
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

void MessageBuilder::appendTwitchBadges()
{
    if (this->twitchChannel == nullptr)
    {
        return;
    }

    auto badgeInfos = MessageBuilder::parseBadgeInfoTag(this->tags);
    auto badges = parseBadgeTag(this->tags);
    appendBadges(this, badges, badgeInfos, this->twitchChannel);
}

void MessageBuilder::appendChatterinoBadges()
{
    if (auto badge = getApp()->getChatterinoBadges()->getBadge({this->userId_}))
    {
        this->emplace<BadgeElement>(*badge,
                                    MessageElementFlag::BadgeChatterino);
    }
}

void MessageBuilder::appendFfzBadges()
{
    for (const auto &badge :
         getApp()->getFfzBadges()->getUserBadges({this->userId_}))
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

void MessageBuilder::appendSeventvBadges()
{
    if (auto badge = getApp()->getSeventvBadges()->getBadge({this->userId_}))
    {
        this->emplace<BadgeElement>(*badge, MessageElementFlag::BadgeSevenTV);
    }
}

Outcome MessageBuilder::tryParseCheermote(const QString &string)
{
    if (this->bitsLeft == 0)
    {
        return Failure;
    }

    const auto *chan = this->getSourceChannel();
    auto cheerOpt = chan->cheerEmote(string);

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

bool MessageBuilder::shouldAddModerationElements() const
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

}  // namespace chatterino
