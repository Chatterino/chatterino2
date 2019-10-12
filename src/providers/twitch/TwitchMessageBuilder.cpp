#include "providers/twitch/TwitchMessageBuilder.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/ignores/IgnoreController.hpp"
#include "controllers/pings/PingController.hpp"
#include "debug/Log.hpp"
#include "messages/Message.hpp"
#include "providers/chatterino/ChatterinoBadges.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "providers/twitch/TwitchChannel.hpp"
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

namespace {

QColor getRandomColor(const QVariant &userId)
{
    static const std::vector<QColor> twitchUsernameColors = {
        {255, 0, 0},      // Red
        {0, 0, 255},      // Blue
        {0, 255, 0},      // Green
        {178, 34, 34},    // FireBrick
        {255, 127, 80},   // Coral
        {154, 205, 50},   // YellowGreen
        {255, 69, 0},     // OrangeRed
        {46, 139, 87},    // SeaGreen
        {218, 165, 32},   // GoldenRod
        {210, 105, 30},   // Chocolate
        {95, 158, 160},   // CadetBlue
        {30, 144, 255},   // DodgerBlue
        {255, 105, 180},  // HotPink
        {138, 43, 226},   // BlueViolet
        {0, 255, 127},    // SpringGreen
    };

    bool ok = true;
    int colorSeed = userId.toInt(&ok);
    if (!ok)
    {
        // We were unable to convert the user ID to an integer, this means Twitch has decided to start using non-integer user IDs
        // Just randomize the users color
        colorSeed = std::rand();
    }

    const auto colorIndex = colorSeed % twitchUsernameColors.size();
    return twitchUsernameColors[colorIndex];
}

QUrl getFallbackHighlightSound()
{
    using namespace chatterino;

    QString path = getSettings()->pathHighlightSound;
    bool fileExists = QFileInfo::exists(path) && QFileInfo(path).isFile();

    // Use fallback sound when checkbox is not checked
    // or custom file doesn't exist
    if (getSettings()->customHighlightSound && fileExists)
    {
        return QUrl::fromLocalFile(path);
    }
    else
    {
        return QUrl("qrc:/sounds/ping2.wav");
    }
}

}  // namespace

namespace chatterino {

TwitchMessageBuilder::TwitchMessageBuilder(
    Channel *_channel, const Communi::IrcPrivateMessage *_ircMessage,
    const MessageParseArgs &_args)
    : channel(_channel)
    , twitchChannel(dynamic_cast<TwitchChannel *>(_channel))
    , ircMessage(_ircMessage)
    , args(_args)
    , tags(this->ircMessage->tags())
    , originalMessage_(_ircMessage->content())
    , action_(_ircMessage->isAction())
{
    this->usernameColor_ = getApp()->themes->messages.textColors.system;
}

TwitchMessageBuilder::TwitchMessageBuilder(
    Channel *_channel, const Communi::IrcMessage *_ircMessage,
    const MessageParseArgs &_args, QString content, bool isAction)
    : channel(_channel)
    , twitchChannel(dynamic_cast<TwitchChannel *>(_channel))
    , ircMessage(_ircMessage)
    , args(_args)
    , tags(this->ircMessage->tags())
    , originalMessage_(content)
    , action_(isAction)
{
    this->usernameColor_ = getApp()->themes->messages.textColors.system;
}

bool TwitchMessageBuilder::isIgnored() const
{
    auto app = getApp();

    // TODO(pajlada): Do we need to check if the phrase is valid first?
    for (const auto &phrase : app->ignores->phrases)
    {
        if (phrase.isBlock() && phrase.isMatch(this->originalMessage_))
        {
            log("Blocking message because it contains ignored phrase {}",
                phrase.getPattern());
            return true;
        }
    }

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
                log("Blocking message because it's from blocked user {}",
                    user.name);
                return true;
            }
        }
    }

    return false;
}

inline QMediaPlayer *getPlayer()
{
    if (isGuiThread())
    {
        static auto player = new QMediaPlayer;
        return player;
    }
    else
    {
        return nullptr;
    }
}

void TwitchMessageBuilder::triggerHighlights()
{
    static QUrl currentPlayerUrl;

    if (this->historicalMessage_)
    {
        // Do nothing. Highlights should not be triggered on historical messages.
        return;
    }

    if (getApp()->pings->isMuted(this->channel->getName()))
    {
        // Do nothing. Pings are muted in this channel.
        return;
    }

    bool hasFocus = (QApplication::focusWidget() != nullptr);
    bool resolveFocus = !hasFocus || getSettings()->highlightAlwaysPlaySound;

    if (this->highlightSound_ && resolveFocus)
    {
        if (auto player = getPlayer())
        {
            // update the media player url if necessary
            if (currentPlayerUrl != this->highlightSoundUrl_)
            {
                player->setMedia(this->highlightSoundUrl_);

                currentPlayerUrl = this->highlightSoundUrl_;
            }

            player->play();
        }
    }

    if (this->highlightAlert_)
    {
        getApp()->windows->sendAlert();
    }
}

MessagePtr TwitchMessageBuilder::build()
{
    // PARSING
    this->userId_ = this->ircMessage->tag("user-id").toString();

    this->parseUsername();

    if (this->userName == this->channel->getName())
    {
        this->senderIsBroadcaster = true;
    }

    this->message().flags.set(MessageFlag::Collapsed);

    // PARSING
    this->parseMessageID();

    this->parseRoomID();

    this->appendChannelName();

    if (this->tags.contains("rm-deleted"))
    {
        this->message().flags.set(MessageFlag::Disabled);
    }

    this->historicalMessage_ = this->tags.contains("historical");

    // timestamp
    if (this->historicalMessage_)
    {
        // This may be architecture dependent(datatype)
        bool customReceived = false;
        qint64 ts =
            this->tags.value("rm-received-ts").toLongLong(&customReceived);
        if (!customReceived)
        {
            ts = this->tags.value("tmi-sent-ts").toLongLong();
        }

        QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(ts);
        this->emplace<TimestampElement>(dateTime.time());
    }
    else
    {
        this->emplace<TimestampElement>();
    }

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

    this->appendUsername();

    //    QString bits;
    auto iterator = this->tags.find("bits");
    if (iterator != this->tags.end())
    {
        this->hasBits_ = true;
        this->bits = iterator.value().toString();
    }

    // twitch emotes
    std::vector<std::tuple<int, EmotePtr, EmoteName>> twitchEmotes;

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
                  return std::get<0>(a) < std::get<0>(b);
              });
    twitchEmotes.erase(std::unique(twitchEmotes.begin(), twitchEmotes.end(),
                                   [](const auto &first, const auto &second) {
                                       return std::get<0>(first) ==
                                              std::get<0>(second);
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
    }

    return this->release();
}

void TwitchMessageBuilder::addWords(
    const QStringList &words,
    const std::vector<std::tuple<int, EmotePtr, EmoteName>> &twitchEmotes)
{
    auto i = int();
    auto currentTwitchEmote = twitchEmotes.begin();

    for (auto word : words)
    {
        // check if it's a twitch emote twitch emote
        while (currentTwitchEmote != twitchEmotes.end() &&
               std::get<0>(*currentTwitchEmote) < i)
        {
            ++currentTwitchEmote;
        }
        if (currentTwitchEmote != twitchEmotes.end() &&
            std::get<0>(*currentTwitchEmote) == i)
        {
            auto emoteImage = std::get<1>(*currentTwitchEmote);
            if (emoteImage == nullptr)
            {
                log("emoteImage nullptr {}",
                    std::get<2>(*currentTwitchEmote).string);
            }
            this->emplace<EmoteElement>(emoteImage,
                                        MessageElementFlag::TwitchEmote);

            i += word.length() + 1;

            int len = std::get<2>(*currentTwitchEmote).string.length();
            currentTwitchEmote++;

            if (len < word.length())
            {
                word = word.mid(len);
                this->message().elements.back()->setTrailingSpace(false);
            }
            else
            {
                continue;
            }
        }

        // split words
        for (auto &variant : getApp()->emotes->emojis.parse(word))
        {
            boost::apply_visitor([&](auto &&arg) { this->addTextOrEmoji(arg); },
                                 variant);
        }

        i += word.size() + 1;
    }
}

void TwitchMessageBuilder::addTextOrEmoji(EmotePtr emote)
{
    this->emplace<EmoteElement>(emote, MessageElementFlag::EmojiAll);
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
    auto link = Link();
    auto textColor = this->action_ ? MessageColor(this->usernameColor_)
                                   : MessageColor(MessageColor::Text);

    if (linkString.isEmpty())
    {
        if (string.startsWith('@'))
        {
            this->emplace<TextElement>(string, MessageElementFlag::BoldUsername,
                                       textColor, FontStyle::ChatMediumBold);
            this->emplace<TextElement>(
                string, MessageElementFlag::NonBoldUsername, textColor);
        }
        else
        {
            this->emplace<TextElement>(string, MessageElementFlag::Text,
                                       textColor);
        }
    }
    else
    {
        this->addLink(string, linkString);
    }

    // if (!linkString.isEmpty()) {
    //    if (getSettings()->lowercaseLink) {
    //        QRegularExpression httpRegex("\\bhttps?://",
    //        QRegularExpression::CaseInsensitiveOption); QRegularExpression
    //        ftpRegex("\\bftps?://",
    //        QRegularExpression::CaseInsensitiveOption); QRegularExpression
    //        getDomain("\\/\\/([^\\/]*)"); QString tempString = string;

    //        if (!string.contains(httpRegex)) {
    //            if (!string.contains(ftpRegex)) {
    //                tempString.insert(0, "http://");
    //            }
    //        }
    //        QString domain = getDomain.match(tempString).captured(1);
    //        string.replace(domain, domain.toLower());
    //    }
    //    link = Link(Link::Url, linkString);
    //    textColor = MessageColor(MessageColor::Link);
    //}
    // if (string.startsWith('@')) {
    //    this->emplace<TextElement>(string, MessageElementFlag::BoldUsername,
    //    textColor,
    //                               FontStyle::ChatMediumBold)  //
    //        ->setLink(link);
    //    this->emplace<TextElement>(string,
    //    MessageElementFlag::NonBoldUsername,
    //                               textColor)  //
    //        ->setLink(link);
    //} else {
    //    this->emplace<TextElement>(string, MessageElementFlag::Text,
    //    textColor)  //
    //        ->setLink(link);
    //}
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

void TwitchMessageBuilder::appendChannelName()
{
    QString channelName("#" + this->channel->getName());
    Link link(Link::Url, this->channel->getName() + "\n" + this->message().id);

    this->emplace<TextElement>(channelName, MessageElementFlag::ChannelName,
                               MessageColor::System)  //
        ->setLink(link);
}

void TwitchMessageBuilder::parseUsernameColor()
{
    const auto iterator = this->tags.find("color");
    if (iterator != this->tags.end())
    {
        if (const auto color = iterator.value().toString(); !color.isEmpty())
        {
            this->usernameColor_ = QColor(color);
            return;
        }
    }

    if (getSettings()->colorizeNicknames && this->tags.contains("user-id"))
    {
        this->usernameColor_ = getRandomColor(this->tags.value("user-id"));
    }
}

void TwitchMessageBuilder::parseUsername()
{
    this->parseUsernameColor();

    // username
    this->userName = this->ircMessage->nick();

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
        // IrcManager::getInstance().getUser().getUserName();
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
    std::vector<std::tuple<int, EmotePtr, EmoteName>> &twitchEmotes)
{
    auto app = getApp();
    const auto &phrases = app->ignores->phrases;
    auto removeEmotesInRange =
        [](int pos, int len,
           std::vector<std::tuple<int, EmotePtr, EmoteName>>
               &twitchEmotes) mutable {
            auto it =
                std::partition(twitchEmotes.begin(), twitchEmotes.end(),
                               [pos, len](const auto &item) {
                                   return !((std::get<0>(item) >= pos) &&
                                            std::get<0>(item) < (pos + len));
                               });
            for (auto copy = it; copy != twitchEmotes.end(); ++copy)
            {
                if (std::get<1>(*copy) == nullptr)
                {
                    log("remem nullptr {}", std::get<2>(*copy).string);
                }
            }
            std::vector<std::tuple<int, EmotePtr, EmoteName>> v(
                it, twitchEmotes.end());
            twitchEmotes.erase(it, twitchEmotes.end());
            return v;
        };

    auto shiftIndicesAfter = [&twitchEmotes](int pos, int by) mutable {
        for (auto &item : twitchEmotes)
        {
            auto &index = std::get<0>(item);
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
                        log("emote null {}", emote.first.string);
                    }
                    twitchEmotes.push_back(std::tuple<int, EmotePtr, EmoteName>{
                        startIndex + pos, emote.second, emote.first});
                }
            }
            pos += word.length() + 1;
        }
    };

    for (const auto &phrase : phrases)
    {
        if (phrase.isBlock())
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
                    if (std::get<1>(tup) == nullptr)
                    {
                        log("v nullptr {}", std::get<2>(tup).string);
                        continue;
                    }
                    QRegularExpression emoteregex(
                        "\\b" + std::get<2>(tup).string + "\\b",
                        QRegularExpression::UseUnicodePropertiesOption);
                    auto _match = emoteregex.match(midExtendedRef);
                    if (_match.hasMatch())
                    {
                        int last = _match.lastCapturedIndex();
                        for (int i = 0; i <= last; ++i)
                        {
                            std::get<0>(tup) = from + _match.capturedStart();
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
            const auto &pattern = phrase.getPattern();
            if (pattern.isEmpty())
            {
                continue;
            }
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
                    if (std::get<1>(tup) == nullptr)
                    {
                        log("v nullptr {}", std::get<2>(tup).string);
                        continue;
                    }
                    QRegularExpression emoteregex(
                        "\\b" + std::get<2>(tup).string + "\\b",
                        QRegularExpression::UseUnicodePropertiesOption);
                    auto match = emoteregex.match(midExtendedRef);
                    if (match.hasMatch())
                    {
                        int last = match.lastCapturedIndex();
                        for (int i = 0; i <= last; ++i)
                        {
                            std::get<0>(tup) = from + match.capturedStart();
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

void TwitchMessageBuilder::parseHighlights()
{
    auto app = getApp();

    auto currentUser = app->accounts->twitch.getCurrent();

    QString currentUsername = currentUser->getUserName();

    if (app->highlights->blacklistContains(this->ircMessage->nick()))
    {
        // Do nothing. We ignore highlights from this user.
        return;
    }

    std::vector<HighlightPhrase> userHighlights =
        app->highlights->highlightedUsers.cloneVector();

    // Highlight because of sender
    for (const HighlightPhrase &userHighlight : userHighlights)
    {
        if (!userHighlight.isMatch(this->ircMessage->nick()))
        {
            continue;
        }
        log("Highlight because user {} sent a message",
            this->ircMessage->nick());
        if (!this->highlightVisual_)
        {
            this->highlightVisual_ = true;
            this->message().flags.set(MessageFlag::Highlighted);
            this->message().highlightColor = userHighlight.getColor();
        }

        if (userHighlight.hasAlert())
        {
            this->highlightAlert_ = true;
        }

        if (userHighlight.hasSound())
        {
            this->highlightSound_ = true;
            // Use custom sound if set, otherwise use the fallback sound
            if (userHighlight.hasCustomSound())
            {
                this->highlightSoundUrl_ = userHighlight.getSoundUrl();
            }
            else
            {
                this->highlightSoundUrl_ = getFallbackHighlightSound();
            }
        }

        if (this->highlightAlert_ && this->highlightSound_)
        {
            // Usernames "beat" highlight phrases: Once a username highlight
            // has been applied, no further highlight phrases will be checked
            return;
        }
    }

    if (this->ircMessage->nick() == currentUsername)
    {
        // Do nothing. Highlights cannot be triggered by yourself
        return;
    }

    // TODO: This vector should only be rebuilt upon highlights being changed
    // fourtf: should be implemented in the HighlightsController
    std::vector<HighlightPhrase> activeHighlights =
        app->highlights->phrases.cloneVector();

    if (getSettings()->enableSelfHighlight && currentUsername.size() > 0)
    {
        HighlightPhrase selfHighlight(
            currentUsername, getSettings()->enableSelfHighlightTaskbar,
            getSettings()->enableSelfHighlightSound, false, false,
            getSettings()->selfHighlightSoundUrl.getValue(),
            QColor(getSettings()->selfHighlightColor));
        activeHighlights.emplace_back(std::move(selfHighlight));
    }

    // Highlight because of message
    for (const HighlightPhrase &highlight : activeHighlights)
    {
        if (!highlight.isMatch(this->originalMessage_))
        {
            continue;
        }

        log("Highlight because {} matches {}", this->originalMessage_,
            highlight.getPattern());
        if (!this->highlightVisual_)
        {
            this->highlightVisual_ = true;
            this->message().flags.set(MessageFlag::Highlighted);
            this->message().highlightColor = highlight.getColor();
        }

        if (highlight.hasAlert())
        {
            this->highlightAlert_ = true;
        }

        // Only set highlightSound_ if it hasn't been set by username
        // highlights already.
        if (highlight.hasSound() && !this->highlightSound_)
        {
            this->highlightSound_ = true;

            // Use custom sound if set, otherwise use fallback sound
            if (highlight.hasCustomSound())
            {
                this->highlightSoundUrl_ = highlight.getSoundUrl();
            }
            else
            {
                this->highlightSoundUrl_ = getFallbackHighlightSound();
            }
        }

        if (this->highlightAlert_ && this->highlightSound_)
        {
            // Break once the first highlight has been set. If a message would
            // trigger multiple highlights, only the first one from the list
            // will be applied.
            break;
        }
    }

    // Highlight because it's a whisper
    if (this->args.isReceivedWhisper && getSettings()->enableWhisperHighlight)
    {
        if (getSettings()->enableWhisperHighlightTaskbar)
        {
            this->highlightAlert_ = true;
        }
        if (getSettings()->enableWhisperHighlightSound)
        {
            this->highlightSound_ = true;

            // Use custom sound is set, otherwise use fallback
            if (!getSettings()->whisperHighlightSoundUrl.getValue().isEmpty())
            {
                this->highlightSoundUrl_ =
                    QUrl(getSettings()->whisperHighlightSoundUrl.getValue());
            }
            else
            {
                this->highlightSoundUrl_ = getFallbackHighlightSound();
            }
        }
        if (!this->highlightVisual_)
        {
            this->highlightVisual_ = true;
            this->message().flags.set(MessageFlag::Highlighted);
            this->message().highlightColor =
                QColor(getSettings()->whisperHighlightColor);
        }
    }
}

void TwitchMessageBuilder::appendTwitchEmote(
    const QString &emote,
    std::vector<std::tuple<int, EmotePtr, EmoteName>> &vec,
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
        auto tup = std::tuple<int, EmotePtr, EmoteName>{
            start, app->emotes->twitch.getOrCreateEmote(id, name), name};
        if (std::get<1>(tup) == nullptr)
        {
            log("nullptr {}", std::get<2>(tup).string);
        }
        vec.push_back(std::move(tup));
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
    }

    if (emote)
    {
        this->emplace<EmoteElement>(emote.get(), flags);
        return Success;
    }

    return Failure;
}

// fourtf: this is ugly
void TwitchMessageBuilder::appendTwitchBadges()
{
    if (this->twitchChannel == nullptr)
    {
        return;
    }

    auto app = getApp();

    auto iterator = this->tags.find("badges");
    if (iterator == this->tags.end())
        return;

    for (QString badge : iterator.value().toString().split(','))
    {
        if (badge.startsWith("bits/"))
        {
            QString cheerAmount = badge.mid(5);
            QString tooltip = QString("Twitch cheer ") + cheerAmount;

            // Try to fetch channel-specific bit badge
            try
            {
                if (twitchChannel)
                    if (const auto &_badge = this->twitchChannel->twitchBadge(
                            "bits", cheerAmount))
                    {
                        this->emplace<BadgeElement>(
                                _badge.get(), MessageElementFlag::BadgeVanity)
                            ->setTooltip(tooltip);
                        continue;
                    }
            }
            catch (const std::out_of_range &)
            {
                // Channel does not contain a special bit badge for this version
            }

            // Use default bit badge
            if (auto _badge = this->twitchChannel->globalTwitchBadges().badge(
                    "bits", cheerAmount))
            {
                this->emplace<BadgeElement>(_badge.get(),
                                            MessageElementFlag::BadgeVanity)
                    ->setTooltip(tooltip);
            }
        }
        else if (badge == "staff/1")
        {
            this->emplace<ImageElement>(
                    Image::fromPixmap(getResources().twitch.staff),
                    MessageElementFlag::BadgeGlobalAuthority)
                ->setTooltip("Twitch Staff");
        }
        else if (badge == "admin/1")
        {
            this->emplace<ImageElement>(
                    Image::fromPixmap(getResources().twitch.admin),
                    MessageElementFlag::BadgeGlobalAuthority)
                ->setTooltip("Twitch Admin");
        }
        else if (badge == "global_mod/1")
        {
            this->emplace<ImageElement>(
                    Image::fromPixmap(getResources().twitch.globalmod),
                    MessageElementFlag::BadgeGlobalAuthority)
                ->setTooltip("Twitch Global Moderator");
        }
        else if (badge == "moderator/1")
        {
            if (auto customModBadge = this->twitchChannel->ffzCustomModBadge())
            {
                this->emplace<ModBadgeElement>(
                        customModBadge.get(),
                        MessageElementFlag::BadgeChannelAuthority)
                    ->setTooltip((*customModBadge)->tooltip.string);
                continue;
            }
            this->emplace<ImageElement>(
                    Image::fromPixmap(getResources().twitch.moderator),
                    MessageElementFlag::BadgeChannelAuthority)
                ->setTooltip("Twitch Channel Moderator");
        }
        else if (badge == "vip/1")
        {
            this->emplace<ImageElement>(
                    Image::fromPixmap(getResources().twitch.vip),
                    MessageElementFlag::BadgeChannelAuthority)
                ->setTooltip("VIP");
        }
        else if (badge == "broadcaster/1")
        {
            this->emplace<ImageElement>(
                    Image::fromPixmap(getResources().twitch.broadcaster),
                    MessageElementFlag::BadgeChannelAuthority)
                ->setTooltip("Twitch Broadcaster");
        }
        else if (badge == "turbo/1")
        {
            this->emplace<ImageElement>(
                    Image::fromPixmap(getResources().twitch.turbo),
                    MessageElementFlag::BadgeVanity)
                ->setTooltip("Twitch Turbo Subscriber");
        }
        else if (badge == "premium/1")
        {
            this->emplace<ImageElement>(
                    Image::fromPixmap(getResources().twitch.prime),
                    MessageElementFlag::BadgeVanity)
                ->setTooltip("Twitch Prime Subscriber");
        }
        else if (badge.startsWith("partner/"))
        {
            int index = badge.midRef(8).toInt();
            switch (index)
            {
                case 1: {
                    this->emplace<ImageElement>(
                            Image::fromPixmap(getResources().twitch.verified,
                                              0.25),
                            MessageElementFlag::BadgeVanity)
                        ->setTooltip("Twitch Verified");
                }
                break;
                default: {
                    printf("[TwitchMessageBuilder] Unhandled partner badge "
                           "index: %d\n",
                           index);
                }
                break;
            }
        }
        else if (badge.startsWith("subscriber/"))
        {
            if (auto badgeEmote = this->twitchChannel->twitchBadge(
                    "subscriber", badge.mid(11)))
            {
                this->emplace<BadgeElement>(
                        badgeEmote.get(), MessageElementFlag::BadgeSubscription)
                    ->setTooltip((*badgeEmote)->tooltip.string);
                continue;
            }

            // use default subscriber badge if custom one not found
            this->emplace<ImageElement>(
                    Image::fromPixmap(getResources().twitch.subscriber, 0.25),
                    MessageElementFlag::BadgeSubscription)
                ->setTooltip("Twitch Subscriber");
        }
        else
        {
            auto splits = badge.split('/');
            if (splits.size() != 2)
                continue;

            if (auto badgeEmote =
                    this->twitchChannel->twitchBadge(splits[0], splits[1]))
            {
                this->emplace<BadgeElement>(badgeEmote.get(),
                                            MessageElementFlag::BadgeVanity)
                    ->setTooltip((*badgeEmote)->tooltip.string);
                continue;
            }
            if (auto _badge = this->twitchChannel->globalTwitchBadges().badge(
                    splits[0], splits[1]))
            {
                this->emplace<BadgeElement>(_badge.get(),
                                            MessageElementFlag::BadgeVanity)
                    ->setTooltip((*_badge)->tooltip.string);
                continue;
            }
        }
    }
}

void TwitchMessageBuilder::appendChatterinoBadges()
{
    if (auto badge = getApp()->chatterinoBadges->getBadge({this->userId_}))
    {
        this->emplace<BadgeElement>(*badge,
                                    MessageElementFlag::BadgeChatterino);
    }
}

Outcome TwitchMessageBuilder::tryParseCheermote(const QString &string)
{
    auto cheerOpt = this->twitchChannel->cheerEmote(string);
    if (!cheerOpt)
    {
        return Failure;
    }
    auto &cheerEmote = *cheerOpt;
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
        this->emplace<TextElement>(this->bits, MessageElementFlag::BitsAmount,
                                   cheerEmote.color);
    }
    return Success;
}
}  // namespace chatterino
