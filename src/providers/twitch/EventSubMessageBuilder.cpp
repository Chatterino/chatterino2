#include "providers/twitch/EventSubMessageBuilder.hpp"

#include "Application.hpp"
#include "common/LinkParser.hpp"
#include "common/Literals.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
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

using namespace chatterino;
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

}  // namespace

EventSubMessageBuilder::EventSubMessageBuilder(
    const eventsub::payload::channel_chat_message::v1::Payload &_payload,
    const MessageParseArgs &_args)
    : payload(_payload)
    , channel(getApp()->twitch->getChannelOrEmptyByID(
          QString::fromStdString(this->payload.event.broadcasterUserID)))
    , args(_args)
    , originalMessage(QString::fromStdString(this->payload.event.message.text))
    , twitchChannel(dynamic_cast<TwitchChannel *>(this->channel.get()))
{
}

MessagePtr EventSubMessageBuilder::build()
{
    this->emplace<TextElement>("ES", MessageElementFlag::Text,
                               MessageColor::Text);
    // Parse sender
    this->userId_ = QString::fromStdString(this->payload.event.chatterUserID);
    this->userName =
        QString::fromStdString(this->payload.event.chatterUserLogin);
    this->message().loginName = this->userName;
    QString displayName =
        QString::fromStdString(this->payload.event.chatterUserName).trimmed();
    if (QString::compare(displayName, this->userName, Qt::CaseInsensitive) == 0)
    {
        this->message().displayName = displayName;
    }
    else
    {
        this->message().displayName = this->userName;
        this->message().localizedName = displayName;
    }

    // Parse channel
    this->roomID_ =
        QString::fromStdString(this->payload.event.broadcasterUserID);

    this->parseUsernameColor();

    // TODO: stylize

    if (this->userName == this->channel->getName())
    {
        this->senderIsBroadcaster = true;
    }

    this->message().channelName = this->channel->getName();

    this->message().id = QString::fromStdString(this->payload.event.messageID);

    // TODO: Handle channel point reward, since it must be appended before any other element

    {
        // appendChannelName
        QString channelName("#" + this->channel->getName());
        Link link(Link::JumpToChannel, this->channel->getName());

        this->emplace<TextElement>(channelName, MessageElementFlag::ChannelName,
                                   MessageColor::System)
            ->setLink(link);
    }

    if (this->tags.contains("rm-deleted"))
    {
        this->message().flags.set(MessageFlag::Disabled);
    }

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
    // TODO: No server received time available
    this->message().serverReceivedTime = QDateTime::currentDateTime();
    this->emplace<TimestampElement>(this->message().serverReceivedTime.time());

    if (this->shouldAddModerationElements())
    {
        this->emplace<TwitchModerationElement>();
    }

    this->appendTwitchBadges();

    this->appendChatterinoBadges();
    this->appendFfzBadges();
    this->appendSeventvBadges();

    qDebug() << "username:" << this->userName;

    // TODO: stylized username
    this->emplace<TextElement>(this->userName, MessageElementFlag::Username,
                               this->usernameColor, FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, this->message().displayName});

    //    QString bits;
    auto iterator = this->tags.find("bits");
    if (iterator != this->tags.end())
    {
        this->hasBits_ = true;
        this->bitsLeft = iterator.value().toInt();
        this->bits = iterator.value().toString();
    }

    // words
    this->addWords();

    // stylizeUsername
    auto localizedName =
        QString::fromStdString(this->payload.event.chatterUserName);
    bool hasLocalizedName = !localizedName.isEmpty();

    // The full string that will be rendered in the chat widget
    QString stylizedUsername;

    switch (getSettings()->usernameDisplayMode.getValue())
    {
        case UsernameDisplayMode::Username: {
            stylizedUsername = this->userName;
        }
        break;

        case UsernameDisplayMode::LocalizedName: {
            if (hasLocalizedName)
            {
                stylizedUsername = localizedName;
            }
            else
            {
                stylizedUsername = this->userName;
            }
        }
        break;

        default:
        case UsernameDisplayMode::UsernameAndLocalizedName: {
            if (hasLocalizedName)
            {
                stylizedUsername = this->userName + "(" + localizedName + ")";
            }
            else
            {
                stylizedUsername = this->userName;
            }
        }
        break;
    }

    if (auto nicknameText = getSettings()->matchNickname(stylizedUsername))
    {
        stylizedUsername = *nicknameText;
    }

    this->message().messageText = this->originalMessage;
    this->message().searchText = stylizedUsername + " " +
                                 this->message().localizedName + " " +
                                 this->userName + ": " + this->originalMessage;

    // TODO: highlights
    // this->parseHighlights();

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

void EventSubMessageBuilder::addTextOrEmoji(EmotePtr emote)
{
    MessageBuilder::addTextOrEmoji(emote);
}

void EventSubMessageBuilder::addTextOrEmoji(const QString &string_)
{
    auto string = QString(string_);

    // TODO: Handle cheermote?

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

void EventSubMessageBuilder::addWords()
{
    qDebug() << "addWords";
    for (const auto &fragment : this->payload.event.message.fragments)
    {
        // We can trim the string here since we add a space between elements by default
        auto text = QString::fromStdString(fragment.text).trimmed();
        qDebug() << "XXX: Fragment:" << text;

        if (fragment.type == "text")
        {
            auto words = text.split(' ');

            for (const auto &word : words)
            {
                if (word.isEmpty())
                {
                    continue;
                }

                // split words
                for (auto &variant :
                     getIApp()->getEmotes()->getEmojis()->parse(word))
                {
                    boost::apply_visitor(
                        [&](auto &&arg) {
                            this->addTextOrEmoji(arg);
                        },
                        variant);
                }
            }
        }
        else if (fragment.type == "emote")
        {
            // Twitch emote
            if (fragment.emote.has_value())
            {
                const auto &emote = *fragment.emote;
                auto emoteID = QString::fromStdString(emote.id);
                const auto &emoteName = text;

                auto emoteImage =
                    getIApp()->getEmotes()->getTwitchEmotes()->getOrCreateEmote(
                        EmoteId{emoteID}, EmoteName{emoteName});

                this->emplace<EmoteElement>(emoteImage,
                                            MessageElementFlag::TwitchEmote,
                                            MessageColor::Text);
            }
            else
            {
                qDebug() << "EMOTE TYPE BUT NO EMOTE??";
            }
        }
        else
        {
            qDebug() << "XXX: Unhandled fragment type"
                     << QString::fromStdString(fragment.type);
            this->emplace<TextElement>(text, MessageElementFlag::Text,
                                       MessageColor::Text);
        }
    }
}

void EventSubMessageBuilder::parseThread()
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
                "@" + usernameText + ":", MessageElementFlag::RepliedMessage,
                threadRoot->usernameColor, FontStyle::ChatMediumSmall)
            ->setLink({Link::UserInfo, threadRoot->displayName});

        this->emplace<SingleLineTextElement>(
                threadRoot->messageText,
                MessageElementFlags({MessageElementFlag::RepliedMessage,
                                     MessageElementFlag::Text}),
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
            QString body;

            this->emplace<ReplyCurveElement>();
            this->emplace<TextElement>(
                "Replying to", MessageElementFlag::RepliedMessage,
                MessageColor::System, FontStyle::ChatMediumSmall);

            // TODO
            if (false /*this->isIgnoredReply()*/)
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

void EventSubMessageBuilder::parseUsernameColor()
{
    const auto *userData = getIApp()->getUserData();
    assert(userData != nullptr);

    if (const auto &user = userData->getUser(this->userId_))
    {
        if (user->color)
        {
            this->usernameColor = user->color.value();
            return;
        }
    }

    if (!this->payload.event.color.empty())
    {
        this->usernameColor =
            QColor(QString::fromStdString(this->payload.event.color));
        this->message().usernameColor = this->usernameColor;
        return;
    }

    if (getSettings()->colorizeNicknames)
    {
        this->usernameColor = getRandomColor(this->userId_);
        this->message().usernameColor = this->usernameColor;
    }
}

void EventSubMessageBuilder::runIgnoreReplaces(
    std::vector<TwitchEmoteOccurrence> &twitchEmotes)
{
    auto phrases = getSettings()->ignoredMessages.readOnly();
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
        std::vector<TwitchEmoteOccurrence> v(it, twitchEmotes.end());
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
                                         const auto &midrepl,
                                         int startIndex) mutable {
        if (!phrase.containsEmote())
        {
            return;
        }

        auto words = midrepl.split(' ');
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
                    twitchEmotes.push_back(TwitchEmoteOccurrence{
                        startIndex + pos,
                        startIndex + pos + (int)emote.first.string.length(),
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
            while ((from = this->originalMessage.indexOf(regex, from,
                                                         &match)) != -1)
            {
                int len = match.capturedLength();
                auto vret = removeEmotesInRange(from, len, twitchEmotes);
                auto mid = this->originalMessage.mid(from, len);
                mid.replace(regex, phrase.getReplace());

                int midsize = mid.size();
                this->originalMessage.replace(from, len, mid);
                int pos1 = from;
                while (pos1 > 0)
                {
                    if (this->originalMessage[pos1 - 1] == ' ')
                    {
                        break;
                    }
                    --pos1;
                }
                int pos2 = from + midsize;
                while (pos2 < this->originalMessage.length())
                {
                    if (this->originalMessage[pos2] == ' ')
                    {
                        break;
                    }
                    ++pos2;
                }

                shiftIndicesAfter(from + len, midsize - len);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                auto midExtendedRef =
                    QStringView{this->originalMessage}.mid(pos1, pos2 - pos1);
#else
                auto midExtendedRef =
                    this->originalMessage.midRef(pos1, pos2 - pos1);
#endif

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
            while ((from = this->originalMessage.indexOf(
                        pattern, from, phrase.caseSensitivity())) != -1)
            {
                int len = pattern.size();
                auto vret = removeEmotesInRange(from, len, twitchEmotes);
                auto replace = phrase.getReplace();

                int replacesize = replace.size();
                this->originalMessage.replace(from, len, replace);

                int pos1 = from;
                while (pos1 > 0)
                {
                    if (this->originalMessage[pos1 - 1] == ' ')
                    {
                        break;
                    }
                    --pos1;
                }
                int pos2 = from + replacesize;
                while (pos2 < this->originalMessage.length())
                {
                    if (this->originalMessage[pos2] == ' ')
                    {
                        break;
                    }
                    ++pos2;
                }

                shiftIndicesAfter(from + len, replacesize - len);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                auto midExtendedRef =
                    QStringView{this->originalMessage}.mid(pos1, pos2 - pos1);
#else
                auto midExtendedRef =
                    this->originalMessage.midRef(pos1, pos2 - pos1);
#endif

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

Outcome EventSubMessageBuilder::tryAppendEmote(const EmoteName &name)
{
    auto *app = getIApp();

    const auto &globalBttvEmotes = app->getBttvEmotes();
    const auto &globalFfzEmotes = app->getFfzEmotes();
    const auto &globalSeventvEmotes = app->getSeventvEmotes();

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

std::optional<EmotePtr> EventSubMessageBuilder::getTwitchBadge(
    const Badge &badge) const
{
    if (auto channelBadge =
            this->twitchChannel->twitchBadge(badge.key_, badge.value_))
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

void EventSubMessageBuilder::appendTwitchBadges()
{
    if (this->twitchChannel == nullptr)
    {
        return;
    }

    for (const auto &rawBadge : this->payload.event.badges)
    {
        const auto key = QString::fromStdString(rawBadge.setID);
        const auto value = QString::fromStdString(rawBadge.id);

        Badge badge(key, value);

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
                        *customModBadge,
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
                        *customVipBadge,
                        MessageElementFlag::BadgeChannelAuthority)
                    ->setTooltip((*customVipBadge)->tooltip.string);
                // early out, since we have to add a custom badge element here
                continue;
            }
        }
        else if (badge.flag_ == MessageElementFlag::BadgeSubscription)
        {
            if (!rawBadge.info.empty())
            {
                // badge.value_ is 4 chars long if user is subbed on higher tier
                // (tier + amount of months with leading zero if less than 100)
                // e.g. 3054 - tier 3 4,5-year sub. 2108 - tier 2 9-year sub
                const auto &subTier =
                    badge.value_.length() > 3 ? badge.value_.at(0) : '1';
                const auto subMonths = QString::fromStdString(rawBadge.info);
                tooltip +=
                    QString(" (%1%2 months)")
                        .arg(subTier != '1' ? QString("Tier %1, ").arg(subTier)
                                            : "")
                        .arg(subMonths);
            }
        }
        else if (badge.flag_ == MessageElementFlag::BadgePredictions)
        {
            if (!rawBadge.info.empty())
            {
                auto info = QString::fromStdString(rawBadge.info);
                // TODO: is this replace necessary for eventsub stuff?
                auto predictionText =
                    info.replace(R"(\s)", " ")  // standard IRC escapes
                        .replace(R"(\:)", ";")
                        .replace(R"(\\)", R"(\)")
                        .replace("⸝", ",");  // twitch's comma escape
                // Careful, the first character is RIGHT LOW PARAPHRASE BRACKET or U+2E1D, which just looks like a comma

                tooltip = QString("Predicted %1").arg(predictionText);
            }
        }

        this->emplace<BadgeElement>(*badgeEmote, badge.flag_)
            ->setTooltip(tooltip);
    }

    // TODO: implement
    /*
    auto badgeInfos = TwitchMessageBuilder::parseBadgeInfoTag(this->tags);
    auto badges = SharedMessageBuilder::parseBadgeTag(this->tags);

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
                        *customModBadge,
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

        this->emplace<BadgeElement>(*badgeEmote, badge.flag_)
            ->setTooltip(tooltip);
    }

    this->message().badges = badges;
    this->message().badgeInfos = badgeInfos;
    */
}

void EventSubMessageBuilder::appendChatterinoBadges()
{
    if (auto badge =
            getIApp()->getChatterinoBadges()->getBadge({this->userId_}))
    {
        this->emplace<BadgeElement>(*badge,
                                    MessageElementFlag::BadgeChatterino);
    }
}

void EventSubMessageBuilder::appendFfzBadges()
{
    for (const auto &badge :
         getIApp()->getFfzBadges()->getUserBadges({this->userId_}))
    {
        this->emplace<FfzBadgeElement>(
            badge.emote, MessageElementFlag::BadgeFfz, badge.color);
    }
}

void EventSubMessageBuilder::appendSeventvBadges()
{
    if (auto badge = getIApp()->getSeventvBadges()->getBadge({this->userId_}))
    {
        this->emplace<BadgeElement>(*badge, MessageElementFlag::BadgeSevenTV);
    }
}

Outcome EventSubMessageBuilder::tryParseCheermote(const QString &string)
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

bool EventSubMessageBuilder::shouldAddModerationElements() const
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

void EventSubMessageBuilder::setThread(std::shared_ptr<MessageThread> thread)
{
    this->thread_ = std::move(thread);
}

void EventSubMessageBuilder::setParent(MessagePtr parent)
{
    this->parent_ = std::move(parent);
}

void EventSubMessageBuilder::setMessageOffset(int offset)
{
    this->messageOffset_ = offset;
}

}  // namespace chatterino
