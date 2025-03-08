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
#include "providers/twitch/pubsubmessages/AutoMod.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrc.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "providers/twitch/TwitchUsers.hpp"
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
#include "util/Variant.hpp"
#include "widgets/Window.hpp"

#include <boost/variant.hpp>
#include <QApplication>
#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QStringBuilder>
#include <QTimeZone>

#include <algorithm>
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

const QRegularExpression SPACE_REGEX("\\s");

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

    size_t i = 0;
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
                               const QUrl &customSoundUrl, bool windowAlert)
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
        QUrl soundUrl = customSoundUrl;
        if (soundUrl.isEmpty())
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

EmotePtr makeSharedChatBadge(const QString &sourceName)
{
    return std::make_shared<Emote>(Emote{
        .name = EmoteName{},
        .images = ImageSet{Image::fromResourcePixmap(
            getResources().twitch.sharedChat, 0.25)},
        .tooltip = Tooltip{"Shared Message" +
                           (sourceName.isEmpty() ? "" : " from " + sourceName)},
        .homePage = Url{"https://link.twitch.tv/SharedChatViewer"},
    });
}

std::tuple<std::optional<EmotePtr>, MessageElementFlags, bool> parseEmote(
    TwitchChannel *twitchChannel, const EmoteName &name)
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

    std::optional<EmotePtr> emote{};

    if (twitchChannel != nullptr)
    {
        // Check for channel emotes

        emote = twitchChannel->ffzEmote(name);
        if (emote)
        {
            return {
                emote,
                MessageElementFlag::FfzEmote,
                false,
            };
        }

        emote = twitchChannel->bttvEmote(name);
        if (emote)
        {
            return {
                emote,
                MessageElementFlag::BttvEmote,
                false,
            };
        }

        emote = twitchChannel->seventvEmote(name);
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
        text.split(SPACE_REGEX, Qt::SkipEmptyParts);
    for (const auto &word : textFragments)
    {
        auto link = linkparser::parse(word);
        if (link)
        {
            this->addLink(*link, word);
            continue;
        }

        this->appendOrEmplaceText(word, MessageColor::System);
    }
    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);
    this->message().messageText = text;
    this->message().searchText = text;
}

MessagePtrMut MessageBuilder::makeSystemMessageWithUser(
    const QString &text, const QString &loginName, const QString &displayName,
    const MessageColor &userColor, const QTime &time)
{
    MessageBuilder builder;
    builder.emplace<TimestampElement>(time);

    const auto textFragments = text.split(SPACE_REGEX, Qt::SkipEmptyParts);
    for (const auto &word : textFragments)
    {
        if (word == displayName)
        {
            builder.emplace<MentionElement>(displayName, loginName,
                                            MessageColor::System, userColor);
            continue;
        }

        builder.appendOrEmplaceText(word, MessageColor::System);
    }

    builder->flags.set(MessageFlag::System);
    builder->flags.set(MessageFlag::DoNotTriggerNotification);
    builder->messageText = text;
    builder->searchText = text;

    return builder.release();
}

MessagePtrMut MessageBuilder::makeSubgiftMessage(const QString &text,
                                                 const QVariantMap &tags,
                                                 const QTime &time)
{
    MessageBuilder builder;
    builder.emplace<TimestampElement>(time);

    auto gifterLogin = tags.value("login").toString();
    auto gifterDisplayName = tags.value("display-name").toString();
    if (gifterDisplayName.isEmpty())
    {
        gifterDisplayName = gifterLogin;
    }
    MessageColor gifterColor = MessageColor::System;
    if (auto colorTag = tags.value("color").value<QColor>(); colorTag.isValid())
    {
        gifterColor = MessageColor(colorTag);
    }

    auto recipientLogin =
        tags.value("msg-param-recipient-user-name").toString();
    if (recipientLogin.isEmpty())
    {
        recipientLogin = tags.value("msg-param-recipient-name").toString();
    }
    auto recipientDisplayName =
        tags.value("msg-param-recipient-display-name").toString();
    if (recipientDisplayName.isEmpty())
    {
        recipientDisplayName = recipientLogin;
    }

    const auto textFragments = text.split(SPACE_REGEX, Qt::SkipEmptyParts);
    for (const auto &word : textFragments)
    {
        if (word == gifterDisplayName)
        {
            builder.emplace<MentionElement>(gifterDisplayName, gifterLogin,
                                            MessageColor::System, gifterColor);
            continue;
        }
        if (word.endsWith('!') &&
            word.size() == recipientDisplayName.size() + 1 &&
            word.startsWith(recipientDisplayName))
        {
            builder
                .emplace<MentionElement>(recipientDisplayName, recipientLogin,
                                         MessageColor::System,
                                         MessageColor::System)
                ->setTrailingSpace(false);
            builder.emplace<TextElement>(u"!"_s, MessageElementFlag::Text,
                                         MessageColor::System);
            continue;
        }

        builder.appendOrEmplaceText(word, MessageColor::System);
    }

    builder->flags.set(MessageFlag::System);
    builder->flags.set(MessageFlag::DoNotTriggerNotification);
    builder->messageText = text;
    builder->searchText = text;

    return builder.release();
}

MessageBuilder::MessageBuilder(TimeoutMessageTag, const QString &timeoutUser,
                               const QString &sourceUser,
                               const QString &channel,
                               const QString &systemMessageText, uint32_t times,
                               const QDateTime &time)
    : MessageBuilder()
{
    QString usernameText = systemMessageText.split(" ").at(0);
    QString remainder = systemMessageText.mid(usernameText.length() + 1);
    bool timeoutUserIsFirst =
        usernameText == "You" || timeoutUser == usernameText;
    QString messageText;

    this->emplace<TimestampElement>(time.time());
    this->emplaceSystemTextAndUpdate(usernameText, messageText)
        ->setLink(
            {Link::UserInfo, timeoutUserIsFirst ? timeoutUser : sourceUser});

    auto appendUser = [&](const QString &name) {
        auto pos = remainder.indexOf(name);
        if (pos > 0)
        {
            QString start = remainder.mid(0, pos - 1);
            remainder = remainder.mid(pos + name.length());

            this->emplaceSystemTextAndUpdate(start, messageText);
            auto *el = this->emplaceSystemTextAndUpdate(name, messageText)
                           ->setLink({Link::UserInfo, name});
            if (remainder.startsWith(' '))
            {
                removeFirstQS(remainder);
            }
            else
            {
                assert(messageText.endsWith(' '));
                removeLastQS(messageText);
                el->setTrailingSpace(false);
            }
        }
    };

    if (!sourceUser.isEmpty())
    {
        // the second username in the message
        appendUser(timeoutUserIsFirst ? sourceUser : timeoutUser);
    }

    if (!channel.isEmpty())
    {
        appendUser(channel);
    }

    this->emplaceSystemTextAndUpdate(
        QString("%1 (%2 times)").arg(remainder.trimmed()).arg(times),
        messageText);

    this->message().messageText = messageText;
    this->message().searchText = messageText;
    this->message().serverReceivedTime = time;
}

MessageBuilder::MessageBuilder(TimeoutMessageTag, const QString &username,
                               const QString &durationInSeconds,
                               bool multipleTimes, const QDateTime &time)
    : MessageBuilder()
{
    QString fullText;
    QString text;

    this->emplace<TimestampElement>(time.time());
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
    this->message().flags.set(MessageFlag::ModerationAction);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);
    this->message().timeoutUser = username;

    this->emplaceSystemTextAndUpdate(text, fullText);
    this->message().messageText = fullText;
    this->message().searchText = fullText;
    this->message().serverReceivedTime = time;
}

MessageBuilder::MessageBuilder(const BanAction &action, const QDateTime &time,
                               uint32_t count)
    : MessageBuilder()
{
    auto current = getApp()->getAccounts()->twitch.getCurrent();

    this->emplace<TimestampElement>();
    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::Timeout);
    this->message().flags.set(MessageFlag::ModerationAction);
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
            this->appendOrEmplaceSystemTextAndUpdate("banned", text);
        }
        else
        {
            this->appendOrEmplaceSystemTextAndUpdate(
                QString("timed out for %1").arg(formatTime(action.duration)),
                text);
        }

        if (!action.source.login.isEmpty())
        {
            this->appendOrEmplaceSystemTextAndUpdate("by", text);
            this->emplaceSystemTextAndUpdate(
                    action.source.login + (action.reason.isEmpty() ? "." : ":"),
                    text)
                ->setLink({Link::UserInfo, action.source.login});
        }

        if (!action.reason.isEmpty())
        {
            this->appendOrEmplaceSystemTextAndUpdate(
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
                this->appendOrEmplaceSystemTextAndUpdate(
                    QString("(%1 times)").arg(count), text);
            }
        }
    }

    this->message().messageText = text;
    this->message().searchText = text;

    this->message().serverReceivedTime = time;
}

MessageBuilder::MessageBuilder(const UnbanAction &action, const QDateTime &time)
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

    this->message().serverReceivedTime = time;
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

MessagePtrMut MessageBuilder::release()
{
    std::shared_ptr<Message> ptr;
    this->message_.swap(ptr);
    return ptr;
}

std::weak_ptr<const Message> MessageBuilder::weakOf()
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

bool MessageBuilder::isIgnored(const QString &originalMessage,
                               const QString &userID, const Channel *channel)
{
    return isIgnoredMessage({
        .message = originalMessage,
        .twitchUserID = userID,
        .isMod = channel->isMod(),
        .isBroadcaster = channel->isBroadcaster(),
    });
}

void MessageBuilder::appendOrEmplaceText(const QString &text,
                                         MessageColor color)
{
    auto fallback = [&] {
        this->emplace<TextElement>(text, MessageElementFlag::Text, color);
    };
    if (this->message_->elements.empty())
    {
        fallback();
        return;
    }

    auto *back =
        dynamic_cast<TextElement *>(this->message_->elements.back().get());
    if (!back ||                                         //
        dynamic_cast<MentionElement *>(back) ||          //
        dynamic_cast<LinkElement *>(back) ||             //
        !back->hasTrailingSpace() ||                     //
        back->getFlags() != MessageElementFlag::Text ||  //
        back->color() != color)
    {
        fallback();
        return;
    }

    back->appendText(text);
}

void MessageBuilder::appendOrEmplaceSystemTextAndUpdate(const QString &text,
                                                        QString &toUpdate)
{
    toUpdate.append(text);
    toUpdate.append(' ');
    this->appendOrEmplaceText(text, MessageColor::System);
}

void MessageBuilder::triggerHighlights(const Channel *channel,
                                       const HighlightAlert &alert)
{
    if (!alert.windowAlert && !alert.playSound)
    {
        return;
    }
    actuallyTriggerHighlights(channel->getName(), alert.playSound,
                              alert.customSound, alert.windowAlert);
}

void MessageBuilder::appendChannelPointRewardMessage(
    const ChannelPointReward &reward, bool isMod, bool isBroadcaster)
{
    if (isIgnoredMessage({
            .message = {},
            .twitchUserID = reward.user.id,
            .isMod = isMod,
            .isBroadcaster = isBroadcaster,
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
    if (!reward.user.login.isEmpty())
    {
        this->message().loginName = reward.user.login;
    }

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
    builder.message().flags.set(MessageFlag::ModerationAction);
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
    builder.message().flags.set(MessageFlag::ModerationAction);

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

    auto locale = getSystemLocale();
    subtitle += locale.toCurrencyString(actualAmount, currency);

    auto dt = calculateMessageTime(message);
    MessageBuilder builder(systemMessage, parseTagString(subtitle), dt.time());
    builder->flags.set(MessageFlag::ElevatedMessage);
    return builder.release();
}

std::pair<MessagePtr, MessagePtr> MessageBuilder::makeAutomodMessage(
    const AutomodAction &action, const QString &channelName)
{
    MessageBuilder builder, builder2;

    if (action.reasonCode == PubSubAutoModQueueMessage::Reason::BlockedTerm)
    {
        builder.message().flags.set(MessageFlag::AutoModBlockedTerm);
        builder2.message().flags.set(MessageFlag::AutoModBlockedTerm);
    }

    //
    // Builder for AutoMod message with explanation
    builder.message().id = "automod_" + action.msgID;
    builder.message().loginName = "automod";
    builder.message().channelName = channelName;
    builder.message().flags.set(MessageFlag::PubSub);
    builder.message().flags.set(MessageFlag::ModerationAction);
    builder.message().flags.set(MessageFlag::AutoMod);
    builder.message().flags.set(MessageFlag::AutoModOffendingMessageHeader);

    // AutoMod shield badge
    builder.emplace<BadgeElement>(makeAutoModBadge(),
                                  MessageElementFlag::BadgeChannelAuthority);
    // AutoMod "username"
    builder.emplace<TextElement>("AutoMod:", MessageElementFlag::Text,
                                 AUTOMOD_USER_COLOR, FontStyle::ChatMediumBold);
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
    builder2.message().flags.set(MessageFlag::ModerationAction);
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
        actuallyTriggerHighlights(
            channelName, highlightResult.playSound,
            highlightResult.customSoundUrl.value_or(QUrl{}),
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
            builder.appendOrEmplaceText(info, MessageColor::Text);
        }
        break;
        case AutomodInfoAction::Denied: {
            QString info("Mods have removed your message.");
            text += info;
            builder.appendOrEmplaceText(info, MessageColor::Text);
        }
        break;
        case AutomodInfoAction::Approved: {
            QString info("Mods have accepted your message.");
            text += info;
            builder.appendOrEmplaceText(info, MessageColor::Text);
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

    QString text;
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
            text = QString("%1 removed %2 from the suspicious user list.")
                       .arg(action.updatedByUserDisplayName,
                            action.suspiciousUserDisplayName);
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
            text = QString("%1 added %2 as a monitored suspicious chatter.")
                       .arg(action.updatedByUserDisplayName,
                            action.suspiciousUserDisplayName);
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
            text = QString("%1 added %2 as a restricted suspicious chatter.")
                       .arg(action.updatedByUserDisplayName,
                            action.suspiciousUserDisplayName);
        }
        break;

        default:
            qCDebug(chatterinoTwitch) << "Unexpected suspicious treatment: "
                                      << action.treatmentString;
            break;
    }

    builder->messageText = text;
    builder->searchText = text;
    return builder.release();
}

MessagePtrMut MessageBuilder::makeClearChatMessage(const QDateTime &now,
                                                   const QString &actor,
                                                   uint32_t count)
{
    MessageBuilder builder;
    builder.emplace<TimestampElement>(now.time());
    builder->count = count;
    builder->serverReceivedTime = now;
    builder.message().flags.set(
        MessageFlag::System, MessageFlag::DoNotTriggerNotification,
        MessageFlag::ClearChat, MessageFlag::ModerationAction);

    QString messageText;
    if (actor.isEmpty())
    {
        builder.emplaceSystemTextAndUpdate(
            "Chat has been cleared by a moderator.", messageText);
    }
    else
    {
        builder.message().flags.set(MessageFlag::PubSub);
        builder.emplace<MentionElement>(actor, actor, MessageColor::System,
                                        MessageColor::System);
        messageText = actor + ' ';
        builder.emplaceSystemTextAndUpdate("cleared the chat.", messageText);
        builder->timeoutUser = actor;
    }

    if (count > 1)
    {
        builder.appendOrEmplaceSystemTextAndUpdate(
            '(' % QString::number(count) % u" times)", messageText);
    }

    builder->messageText = messageText;
    builder->searchText = messageText;

    return builder.release();
}

std::pair<MessagePtrMut, HighlightAlert> MessageBuilder::makeIrcMessage(
    /* mutable */ Channel *channel, const Communi::IrcMessage *ircMessage,
    const MessageParseArgs &args, /* mutable */ QString content,
    const QString::size_type messageOffset,
    const std::shared_ptr<MessageThread> &thread, const MessagePtr &parent)
{
    assert(ircMessage != nullptr);
    assert(channel != nullptr);

    auto tags = ircMessage->tags();
    if (args.allowIgnore)
    {
        bool ignored = MessageBuilder::isIgnored(
            content, tags.value("user-id").toString(), channel);
        if (ignored)
        {
            return {};
        }
    }

    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel);

    auto userID = tags.value("user-id").toString();

    MessageBuilder builder;
    builder.parseUsernameColor(tags, userID);
    builder->userID = userID;

    if (args.isAction)
    {
        builder.textColor_ = builder.message_->usernameColor;
        builder->flags.set(MessageFlag::Action);
    }

    builder.parseUsername(ircMessage, twitchChannel,
                          args.trimSubscriberUsername);

    builder->flags.set(MessageFlag::Collapsed);

    bool senderIsBroadcaster = builder->loginName == channel->getName();

    builder->channelName = channel->getName();

    builder.parseMessageID(tags);

    MessageBuilder::parseRoomID(tags, twitchChannel);
    twitchChannel = builder.parseSharedChatInfo(tags, twitchChannel);

    // If it is a reward it has to be appended first
    if (!args.channelPointRewardId.isEmpty())
    {
        assert(twitchChannel != nullptr);
        auto reward =
            twitchChannel->channelPointReward(args.channelPointRewardId);
        if (reward)
        {
            builder.appendChannelPointRewardMessage(*reward, channel->isMod(),
                                                    channel->isBroadcaster());
        }
        builder->flags.set(MessageFlag::RedeemedChannelPointReward);
    }

    builder.appendChannelName(channel);

    if (tags.contains("rm-deleted"))
    {
        builder->flags.set(MessageFlag::Disabled);
    }

    if (tags.contains("msg-id") &&
        tags["msg-id"].toString().split(';').contains("highlighted-message"))
    {
        builder->flags.set(MessageFlag::RedeemedHighlight);
    }

    if (tags.contains("first-msg") && tags["first-msg"].toString() == "1")
    {
        builder->flags.set(MessageFlag::FirstMessage);
    }

    if (tags.contains("pinned-chat-paid-amount"))
    {
        builder->flags.set(MessageFlag::ElevatedMessage);
    }

    if (tags.contains("bits"))
    {
        builder->flags.set(MessageFlag::CheerMessage);
    }

    // reply threads
    builder.parseThread(content, tags, channel, thread, parent);

    // timestamp
    builder->serverReceivedTime = calculateMessageTime(ircMessage);
    builder.emplace<TimestampElement>(builder->serverReceivedTime.time());

    bool shouldAddModerationElements = [&] {
        if (senderIsBroadcaster)
        {
            // You cannot timeout the broadcaster
            return false;
        }

        if (tags.value("user-type").toString() == "mod" &&
            !args.isStaffOrBroadcaster)
        {
            // You cannot timeout moderators UNLESS you are Twitch Staff or the broadcaster of the channel
            return false;
        }

        return true;
    }();
    if (shouldAddModerationElements)
    {
        builder.emplace<TwitchModerationElement>();
    }

    builder.appendTwitchBadges(tags, twitchChannel);

    builder.appendChatterinoBadges(userID);
    builder.appendFfzBadges(twitchChannel, userID);
    builder.appendSeventvBadges(userID);

    builder.appendUsername(tags, args);

    TextState textState{.twitchChannel = twitchChannel};
    QString bits;

    auto iterator = tags.find("bits");
    if (iterator != tags.end())
    {
        textState.hasBits = true;
        textState.bitsLeft = iterator.value().toInt();
        bits = iterator.value().toString();
    }

    // Twitch emotes
    auto twitchEmotes =
        parseTwitchEmotes(tags, content, static_cast<int>(messageOffset));

    // This runs through all ignored phrases and runs its replacements on content
    processIgnorePhrases(*getSettings()->ignoredMessages.readOnly(), content,
                         twitchEmotes);

    std::ranges::sort(twitchEmotes, [](const auto &a, const auto &b) {
        return a.start < b.start;
    });
    auto uniqueEmotes = std::ranges::unique(
        twitchEmotes, [](const auto &first, const auto &second) {
            return first.start == second.start;
        });
    twitchEmotes.erase(uniqueEmotes.begin(), uniqueEmotes.end());

    // words
    QStringList splits = content.split(' ');

    builder.addWords(splits, twitchEmotes, textState);

    QString stylizedUsername =
        stylizeUsername(builder->loginName, builder.message());

    builder->messageText = content;
    builder->searchText = stylizedUsername + " " + builder->localizedName +
                          " " + builder->loginName + ": " + content + " " +
                          builder->searchText;

    // highlights
    HighlightAlert highlight = builder.parseHighlights(tags, content, args);
    if (tags.contains("historical"))
    {
        highlight.playSound = false;
        highlight.windowAlert = false;
    }

    // highlighting incoming whispers if requested per setting
    if (args.isReceivedWhisper && getSettings()->highlightInlineWhispers)
    {
        builder->flags.set(MessageFlag::HighlightedWhisper);
        builder->highlightColor =
            ColorProvider::instance().color(ColorType::Whisper);
    }

    if (!args.isReceivedWhisper && tags.value("msg-id") != "announcement")
    {
        if (thread)
        {
            auto &img = getResources().buttons.replyThreadDark;
            builder
                .emplace<CircularImageElement>(
                    Image::fromResourcePixmap(img, 0.15), 2, Qt::gray,
                    MessageElementFlag::ReplyButton)
                ->setLink({Link::ViewThread, thread->rootId()});
        }
        else
        {
            auto &img = getResources().buttons.replyDark;
            builder
                .emplace<CircularImageElement>(
                    Image::fromResourcePixmap(img, 0.15), 2, Qt::gray,
                    MessageElementFlag::ReplyButton)
                ->setLink({Link::ReplyToMessage, builder->id});
        }
    }

    return {builder.release(), highlight};
}

void MessageBuilder::addEmoji(const EmotePtr &emote)
{
    this->emplace<EmoteElement>(emote, MessageElementFlag::EmojiAll);
}

void MessageBuilder::addTextOrEmote(TextState &state, QString string)
{
    if (state.hasBits && this->tryAppendCheermote(state, string))
    {
        // This string was parsed as a cheermote
        return;
    }

    // TODO: Implement ignored emotes
    // Format of ignored emotes:
    // Emote name: "forsenPuke" - if string in ignoredEmotes
    // Will match emote regardless of source (i.e. bttv, ffz)
    // Emote source + name: "bttv:nyanPls"
    if (this->tryAppendEmote(state.twitchChannel, {string}))
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

            if (state.twitchChannel != nullptr)
            {
                if (auto userColor =
                        state.twitchChannel->getUserColor(username);
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

    if (state.twitchChannel != nullptr && getSettings()->findAllUsernames)
    {
        auto match = allUsernamesMentionRegex.match(string);
        QString username = match.captured(1);

        if (match.hasMatch() &&
            state.twitchChannel->accessChatters()->contains(username))
        {
            auto originalTextColor = textColor;

            if (auto userColor = state.twitchChannel->getUserColor(username);
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

    this->appendOrEmplaceText(string, textColor);
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

void MessageBuilder::parseUsernameColor(const QVariantMap &tags,
                                        const QString &userID)
{
    const auto *userData = getApp()->getUserData();
    assert(userData != nullptr);

    if (const auto &user = userData->getUser(userID))
    {
        if (user->color)
        {
            this->usernameColor_ = user->color.value();
            this->message().usernameColor = this->usernameColor_;
            return;
        }
    }

    const auto iterator = tags.find("color");
    if (iterator != tags.end())
    {
        if (const auto color = iterator.value().toString(); !color.isEmpty())
        {
            this->usernameColor_ = QColor(color);
            this->message().usernameColor = this->usernameColor_;
            return;
        }
    }

    if (getSettings()->colorizeNicknames && tags.contains("user-id"))
    {
        this->usernameColor_ = getRandomColor(tags.value("user-id").toString());
        this->message().usernameColor = this->usernameColor_;
    }
}

void MessageBuilder::parseUsername(const Communi::IrcMessage *ircMessage,
                                   TwitchChannel *twitchChannel,
                                   bool trimSubscriberUsername)
{
    // username
    auto userName = ircMessage->nick();

    if (userName.isEmpty() || trimSubscriberUsername)
    {
        userName = ircMessage->tag("login").toString();
    }

    this->message_->loginName = userName;
    if (twitchChannel != nullptr)
    {
        twitchChannel->setUserColor(userName, this->message_->usernameColor);
    }

    // Update current user color if this is our message
    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (ircMessage->nick() == currentUser->getUserName())
    {
        currentUser->setColor(this->message_->usernameColor);
    }
}

void MessageBuilder::parseMessageID(const QVariantMap &tags)
{
    auto iterator = tags.find("id");

    if (iterator != tags.end())
    {
        this->message().id = iterator.value().toString();
    }
}

QString MessageBuilder::parseRoomID(const QVariantMap &tags,
                                    TwitchChannel *twitchChannel)
{
    if (twitchChannel == nullptr)
    {
        return {};
    }

    auto iterator = tags.find("room-id");

    if (iterator != std::end(tags))
    {
        auto roomID = iterator->toString();
        if (twitchChannel->roomId() != roomID)
        {
            if (twitchChannel->roomId().isEmpty())
            {
                twitchChannel->setRoomId(roomID);
            }
            else
            {
                qCWarning(chatterinoTwitch)
                    << "The room-ID of the received message doesn't match the "
                       "room-ID of the channel - received:"
                    << roomID << "channel:" << twitchChannel->roomId();
            }
        }
        return roomID;
    }

    return {};
}

TwitchChannel *MessageBuilder::parseSharedChatInfo(const QVariantMap &tags,
                                                   TwitchChannel *twitchChannel)
{
    if (!twitchChannel)
    {
        return twitchChannel;
    }

    if (auto it = tags.find("source-room-id"); it != tags.end())
    {
        auto sourceRoom = it.value().toString();
        if (twitchChannel->roomId() != sourceRoom)
        {
            this->message().flags.set(MessageFlag::SharedMessage);

            auto sourceChan =
                getApp()->getTwitch()->getChannelOrEmptyByID(sourceRoom);
            if (sourceChan && !sourceChan->isEmpty())
            {
                // avoid duplicate pings
                this->message().flags.set(
                    MessageFlag::DoNotTriggerNotification);

                auto *chan = dynamic_cast<TwitchChannel *>(sourceChan.get());
                if (chan)
                {
                    return chan;
                }
            }
        }
    }
    return twitchChannel;
}

void MessageBuilder::parseThread(const QString &messageContent,
                                 const QVariantMap &tags,
                                 const Channel *channel,
                                 const std::shared_ptr<MessageThread> &thread,
                                 const MessagePtr &parent)
{
    if (thread)
    {
        // set references
        this->message().replyThread = thread;
        this->message().replyParent = parent;
        thread->addToThread(std::weak_ptr{this->message_});

        if (thread->subscribed())
        {
            this->message().flags.set(MessageFlag::SubscribedThread);
        }

        // enable reply flag
        this->message().flags.set(MessageFlag::ReplyMessage);

        MessagePtr threadRoot;
        if (!parent)
        {
            threadRoot = thread->root();
        }
        else
        {
            threadRoot = parent;
        }

        QString usernameText =
            stylizeUsername(threadRoot->loginName, *threadRoot);

        this->emplace<ReplyCurveElement>();

        // construct reply elements
        this->emplace<TextElement>(
                "Replying to", MessageElementFlag::RepliedMessage,
                MessageColor::System, FontStyle::ChatMediumSmall)
            ->setLink({Link::ViewThread, thread->rootId()});

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
            ->setLink({Link::ViewThread, thread->rootId()});
    }
    else if (tags.find("reply-parent-msg-id") != tags.end())
    {
        // Message is a reply but we couldn't find the original message.
        // Render the message using the additional reply tags

        auto replyDisplayName = tags.find("reply-parent-display-name");
        auto replyBody = tags.find("reply-parent-msg-body");

        if (replyDisplayName != tags.end() && replyBody != tags.end())
        {
            QString body;

            this->emplace<ReplyCurveElement>();
            this->emplace<TextElement>(
                "Replying to", MessageElementFlag::RepliedMessage,
                MessageColor::System, FontStyle::ChatMediumSmall);

            bool ignored = MessageBuilder::isIgnored(
                messageContent, tags.value("reply-parent-user-id").toString(),
                channel);
            if (ignored)
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

HighlightAlert MessageBuilder::parseHighlights(const QVariantMap &tags,
                                               const QString &originalMessage,
                                               const MessageParseArgs &args)
{
    if (getSettings()->isBlacklistedUser(this->message().loginName))
    {
        // Do nothing. We ignore highlights from this user.
        return {};
    }

    auto badges = parseBadgeTag(tags);
    auto [highlighted, highlightResult] = getApp()->getHighlights()->check(
        args, badges, this->message().loginName, originalMessage,
        this->message().flags);

    if (!highlighted)
    {
        return {};
    }

    // This message triggered one or more highlights, act upon the highlight result

    this->message().flags.set(MessageFlag::Highlighted);

    this->message().highlightColor = highlightResult.color;

    if (highlightResult.showInMentions)
    {
        this->message().flags.set(MessageFlag::ShowInMentions);
    }

    auto customSound = [&] {
        if (highlightResult.customSoundUrl)
        {
            return *highlightResult.customSoundUrl;
        }
        return QUrl{};
    }();
    return {
        .customSound = customSound,
        .playSound = highlightResult.playSound,
        .windowAlert = highlightResult.alert,
    };
}

void MessageBuilder::appendChannelName(const Channel *channel)
{
    QString channelName("#" + channel->getName());
    Link link(Link::JumpToChannel, channel->getName());

    this->emplace<TextElement>(channelName, MessageElementFlag::ChannelName,
                               MessageColor::System)
        ->setLink(link);
}

void MessageBuilder::appendUsername(const QVariantMap &tags,
                                    const MessageParseArgs &args)
{
    auto *app = getApp();

    QString username = this->message_->loginName;
    QString localizedName;

    auto iterator = tags.find("display-name");
    if (iterator != tags.end())
    {
        QString displayName =
            parseTagString(iterator.value().toString()).trimmed();

        if (QString::compare(displayName, username, Qt::CaseInsensitive) == 0)
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

    if (args.isSentWhisper)
    {
        // TODO(pajlada): Re-implement
        // userDisplayString +=
        // IrcManager::instance().getUser().getUserName();
    }
    else if (args.isReceivedWhisper)
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
        if (!args.isAction)
        {
            usernameText += ":";
        }

        this->emplace<TextElement>(usernameText, MessageElementFlag::Username,
                                   this->usernameColor_,
                                   FontStyle::ChatMediumBold)
            ->setLink({Link::UserInfo, this->message().displayName});
    }
}

Outcome MessageBuilder::tryAppendEmote(TwitchChannel *twitchChannel,
                                       const EmoteName &name)
{
    auto [emote, flags, zeroWidth] = parseEmote(twitchChannel, name);

    if (!emote)
    {
        return Failure;
    }

    if (zeroWidth && getSettings()->enableZeroWidthEmotes && !this->isEmpty())
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

        auto *asLayered = dynamic_cast<LayeredEmoteElement *>(&this->back());
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

void MessageBuilder::addWords(
    const QStringList &words,
    const std::vector<TwitchEmoteOccurrence> &twitchEmotes, TextState &state)
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
            for (auto variant :
                 getApp()->getEmotes()->getEmojis()->parse(preText))
            {
                boost::apply_visitor(variant::Overloaded{
                                         [&](const EmotePtr &emote) {
                                             this->addEmoji(emote);
                                         },
                                         [&](QString text) {
                                             this->addTextOrEmote(
                                                 state, std::move(text));
                                         },
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
        for (auto variant : getApp()->getEmotes()->getEmojis()->parse(word))
        {
            boost::apply_visitor(variant::Overloaded{
                                     [&](const EmotePtr &emote) {
                                         this->addEmoji(emote);
                                     },
                                     [&](QString text) {
                                         this->addTextOrEmote(state,
                                                              std::move(text));
                                     },
                                 },
                                 variant);
        }

        cursor += word.size() + 1;
    }
}

void MessageBuilder::appendTwitchBadges(const QVariantMap &tags,
                                        TwitchChannel *twitchChannel)
{
    if (twitchChannel == nullptr)
    {
        return;
    }

    if (this->message().flags.has(MessageFlag::SharedMessage))
    {
        const QString sourceId = tags["source-room-id"].toString();
        QString sourceName;
        if (sourceId.isEmpty())
        {
            sourceName = "";
        }
        else if (twitchChannel->roomId() == sourceId)
        {
            sourceName = twitchChannel->getName();
        }
        else
        {
            sourceName =
                getApp()->getTwitchUsers()->resolveID({sourceId})->displayName;
        }

        this->emplace<BadgeElement>(makeSharedChatBadge(sourceName),
                                    MessageElementFlag::BadgeSharedChannel);
    }

    auto badgeInfos = parseBadgeInfoTag(tags);
    auto badges = parseBadgeTag(tags);
    appendBadges(this, badges, badgeInfos, twitchChannel);
}

void MessageBuilder::appendChatterinoBadges(const QString &userID)
{
    if (auto badge = getApp()->getChatterinoBadges()->getBadge({userID}))
    {
        this->emplace<BadgeElement>(*badge,
                                    MessageElementFlag::BadgeChatterino);
    }
}

void MessageBuilder::appendFfzBadges(TwitchChannel *twitchChannel,
                                     const QString &userID)
{
    for (const auto &badge : getApp()->getFfzBadges()->getUserBadges({userID}))
    {
        this->emplace<FfzBadgeElement>(
            badge.emote, MessageElementFlag::BadgeFfz, badge.color);
    }

    if (twitchChannel == nullptr)
    {
        return;
    }

    for (const auto &badge : twitchChannel->ffzChannelBadges(userID))
    {
        this->emplace<FfzBadgeElement>(
            badge.emote, MessageElementFlag::BadgeFfz, badge.color);
    }
}

void MessageBuilder::appendSeventvBadges(const QString &userID)
{
    if (auto badge = getApp()->getSeventvBadges()->getBadge({userID}))
    {
        this->emplace<BadgeElement>(*badge, MessageElementFlag::BadgeSevenTV);
    }
}

Outcome MessageBuilder::tryAppendCheermote(TextState &state,
                                           const QString &string)
{
    if (state.bitsLeft == 0)
    {
        return Failure;
    }

    auto cheerOpt = state.twitchChannel->cheerEmote(string);

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
        if (state.bitsStacked)
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
            this->emplace<TextElement>(QString::number(state.bitsLeft),
                                       MessageElementFlag::BitsAmount,
                                       cheerEmote.color);
        }
        state.bitsStacked = true;
        return Success;
    }

    if (state.bitsLeft >= cheerValue)
    {
        state.bitsLeft -= cheerValue;
    }
    else
    {
        QString newString = string;
        newString.chop(QString::number(cheerValue).length());
        newString += QString::number(cheerValue - state.bitsLeft);

        return this->tryAppendCheermote(state, newString);
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

}  // namespace chatterino
