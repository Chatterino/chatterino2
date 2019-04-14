#include "providers/twitch/TwitchMessageBuilder.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/ignores/IgnoreController.hpp"
#include "debug/Log.hpp"
#include "messages/Message.hpp"
#include "providers/LinkResolver.hpp"
#include "providers/chatterino/ChatterinoBadges.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/IrcHelpers.hpp"
#include "widgets/Window.hpp"

#include <QApplication>
#include <QDebug>
#include <QMediaPlayer>
#include <QStringRef>
#include <boost/variant.hpp>

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
    for (const auto &phrase : app->ignores->phrases.getVector())
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
                }
                log("Blocking message because it's from blocked user {}",
                    user.name);
                return true;
            }
        }
    }

    return false;
}

MessagePtr TwitchMessageBuilder::build()
{
    // PARSING
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

    // timestamp
    bool isPastMsg = this->tags.contains("historical");
    if (isPastMsg)
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

    // highlights
    this->parseHighlights(isPastMsg);

    //    QString bits;
    auto iterator = this->tags.find("bits");
    if (iterator != this->tags.end())
    {
        this->hasBits_ = true;
        //        bits = iterator.value().toString();
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
    auto app = getApp();
    const auto &phrases = app->ignores->phrases.getVector();
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

    this->message().searchText = this->userName + ": " + this->originalMessage_;

    return this->release();
}

void TwitchMessageBuilder::addWords(
    const QStringList &words,
    const std::vector<std::tuple<int, EmotePtr, EmoteName>> &twitchEmotes)
{
    auto i = int();
    auto currentTwitchEmote = twitchEmotes.begin();

    for (const auto &word : words)
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
            currentTwitchEmote++;

            continue;
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
        static QRegularExpression domainRegex(
            R"(^(?:(?:ftp|http)s?:\/\/)?([^\/]+)(?:\/.*)?$)",
            QRegularExpression::CaseInsensitiveOption);

        QString lowercaseLinkString;
        auto match = domainRegex.match(string);
        if (match.isValid())
        {
            lowercaseLinkString = string.mid(0, match.capturedStart(1)) +
                                  match.captured(1).toLower() +
                                  string.mid(match.capturedEnd(1));
        }
        else
        {
            lowercaseLinkString = string;
        }
        link = Link(Link::Url, linkString);

        textColor = MessageColor(MessageColor::Link);
        auto linkMELowercase =
            this->emplace<TextElement>(lowercaseLinkString,
                                       MessageElementFlag::LowercaseLink,
                                       textColor)
                ->setLink(link);
        auto linkMEOriginal =
            this->emplace<TextElement>(string, MessageElementFlag::OriginalLink,
                                       textColor)
                ->setLink(link);

        LinkResolver::getLinkInfo(
            linkString,
            [weakMessage = this->weakOf(), linkMELowercase, linkMEOriginal,
             linkString](QString tooltipText, Link originalLink) {
                auto shared = weakMessage.lock();
                if (!shared)
                {
                    return;
                }
                if (!tooltipText.isEmpty())
                {
                    linkMELowercase->setTooltip(tooltipText);
                    linkMEOriginal->setTooltip(tooltipText);
                }
                if (originalLink.value != linkString &&
                    !originalLink.value.isEmpty())
                {
                    linkMELowercase->setLink(originalLink)->updateLink();
                    linkMEOriginal->setLink(originalLink)->updateLink();
                }
            });
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
        this->messageID = iterator.value().toString();
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
    Link link(Link::Url, this->channel->getName() + "\n" + this->messageID);

    this->emplace<TextElement>(channelName, MessageElementFlag::ChannelName,
                               MessageColor::System)  //
        ->setLink(link);
}

void TwitchMessageBuilder::parseUsername()
{
    auto iterator = this->tags.find("color");
    if (iterator != this->tags.end())
    {
        this->usernameColor_ = QColor(iterator.value().toString());
    }

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
        case UsernameDisplayMode::Username:
        {
            usernameText = username;
        }
        break;

        case UsernameDisplayMode::LocalizedName:
        {
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
        case UsernameDisplayMode::UsernameAndLocalizedName:
        {
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

void TwitchMessageBuilder::parseHighlights(bool isPastMsg)
{
    static auto player = new QMediaPlayer;
    static QUrl currentPlayerUrl;

    auto app = getApp();

    auto currentUser = app->accounts->twitch.getCurrent();

    QString currentUsername = currentUser->getUserName();

    if (this->ircMessage->nick() == currentUsername)
    {
        currentUser->setColor(this->usernameColor_);
        // Do nothing. Highlights cannot be triggered by yourself
        return;
    }

    // update the media player url if necessary
    QUrl highlightSoundUrl;
    if (getSettings()->customHighlightSound)
    {
        highlightSoundUrl =
            QUrl::fromLocalFile(getSettings()->pathHighlightSound.getValue());
    }
    else
    {
        highlightSoundUrl = QUrl("qrc:/sounds/ping2.wav");
    }

    if (currentPlayerUrl != highlightSoundUrl)
    {
        player->setMedia(highlightSoundUrl);

        currentPlayerUrl = highlightSoundUrl;
    }

    // TODO: This vector should only be rebuilt upon highlights being changed
    // fourtf: should be implemented in the HighlightsController
    std::vector<HighlightPhrase> activeHighlights =
        app->highlights->phrases.getVector();
    std::vector<HighlightPhrase> userHighlights =
        app->highlights->highlightedUsers.getVector();

    if (getSettings()->enableSelfHighlight && currentUsername.size() > 0)
    {
        HighlightPhrase selfHighlight(
            currentUsername, getSettings()->enableSelfHighlightTaskbar,
            getSettings()->enableSelfHighlightSound, false);
        activeHighlights.emplace_back(std::move(selfHighlight));
    }

    bool doHighlight = false;
    bool playSound = false;
    bool doAlert = false;

    bool hasFocus = (QApplication::focusWidget() != nullptr);

    if (!app->highlights->blacklistContains(this->ircMessage->nick()))
    {
        for (const HighlightPhrase &highlight : activeHighlights)
        {
            if (highlight.isMatch(this->originalMessage_))
            {
                log("Highlight because {} matches {}", this->originalMessage_,
                    highlight.getPattern());
                doHighlight = true;

                if (highlight.getAlert())
                {
                    doAlert = true;
                }

                if (highlight.getSound())
                {
                    playSound = true;
                }

                if (playSound && doAlert)
                {
                    // Break if no further action can be taken from other
                    // highlights This might change if highlights can have
                    // custom colors/sounds/actions
                    break;
                }
            }
        }
        for (const HighlightPhrase &userHighlight : userHighlights)
        {
            if (userHighlight.isMatch(this->ircMessage->nick()))
            {
                log("Highlight because user {} sent a message",
                    this->ircMessage->nick());
                doHighlight = true;

                if (userHighlight.getAlert())
                {
                    doAlert = true;
                }

                if (userHighlight.getSound())
                {
                    playSound = true;
                }

                if (playSound && doAlert)
                {
                    // Break if no further action can be taken from other
                    // usernames Mostly used for regex stuff
                    break;
                }
            }
        }
        if (this->args.isReceivedWhisper &&
            getSettings()->enableWhisperHighlight)
        {
            if (getSettings()->enableWhisperHighlightTaskbar)
            {
                doAlert = true;
            }
            if (getSettings()->enableWhisperHighlightSound)
            {
                playSound = true;
            }
        }

        this->message().flags.set(MessageFlag::Highlighted, doHighlight);

        if (!isPastMsg)
        {
            if (playSound &&
                (!hasFocus || getSettings()->highlightAlwaysPlaySound))
            {
                player->play();
            }

            if (doAlert)
            {
                getApp()->windows->sendAlert();
            }
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

        auto start = correctPositions[coords.at(0).toInt()];
        auto end = correctPositions[coords.at(1).toInt()];

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
    // Special channels, like /whispers and /channels return here
    // This means they will not render any BTTV or FFZ emotes
    if (this->twitchChannel == nullptr)
    {
        auto *app = getApp();
        const auto &bttvemotes = app->twitch.server->getBttvEmotes();
        const auto &ffzemotes = app->twitch.server->getFfzEmotes();
        auto flags = MessageElementFlags();
        auto emote = boost::optional<EmotePtr>{};
        {  // bttv/ffz emote
            if ((emote = bttvemotes.emote(name)))
            {
                flags = MessageElementFlag::BttvEmote;
            }
            else if ((emote = ffzemotes.emote(name)))
            {
                flags = MessageElementFlag::FfzEmote;
            }
            if (emote)
            {
                this->emplace<EmoteElement>(emote.get(), flags);
                return Success;
            }
        }  // bttv/ffz emote
        return Failure;
    }

    auto flags = MessageElementFlags();
    auto emote = boost::optional<EmotePtr>{};

    if ((emote = this->twitchChannel->globalBttv().emote(name)))
    {
        flags = MessageElementFlag::BttvEmote;
    }
    else if ((emote = this->twitchChannel->bttvEmote(name)))
    {
        flags = MessageElementFlag::BttvEmote;
    }
    else if ((emote = this->twitchChannel->globalFfz().emote(name)))
    {
        flags = MessageElementFlag::FfzEmote;
    }
    else if ((emote = this->twitchChannel->ffzEmote(name)))
    {
        flags = MessageElementFlag::FfzEmote;
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
                    if (const auto &badge = this->twitchChannel->twitchBadge(
                            "bits", cheerAmount))
                    {
                        this->emplace<EmoteElement>(
                                badge.get(), MessageElementFlag::BadgeVanity)
                            ->setTooltip(tooltip);
                        continue;
                    }
            }
            catch (const std::out_of_range &)
            {
                // Channel does not contain a special bit badge for this version
            }

            // Use default bit badge
            if (auto badge = this->twitchChannel->globalTwitchBadges().badge(
                    "bits", cheerAmount))
            {
                this->emplace<EmoteElement>(badge.get(),
                                            MessageElementFlag::BadgeVanity)
                    ->setTooltip(tooltip);
            }
        }
        else if (badge == "staff/1")
        {
            this->emplace<ImageElement>(
                    Image::fromPixmap(app->resources->twitch.staff),
                    MessageElementFlag::BadgeGlobalAuthority)
                ->setTooltip("Twitch Staff");
        }
        else if (badge == "admin/1")
        {
            this->emplace<ImageElement>(
                    Image::fromPixmap(app->resources->twitch.admin),
                    MessageElementFlag::BadgeGlobalAuthority)
                ->setTooltip("Twitch Admin");
        }
        else if (badge == "global_mod/1")
        {
            this->emplace<ImageElement>(
                    Image::fromPixmap(app->resources->twitch.globalmod),
                    MessageElementFlag::BadgeGlobalAuthority)
                ->setTooltip("Twitch Global Moderator");
        }
        else if (badge == "moderator/1")
        {
            if (auto customModBadge = this->twitchChannel->ffzCustomModBadge())
            {
                this->emplace<EmoteElement>(
                        customModBadge.get(),
                        MessageElementFlag::BadgeChannelAuthority)
                    ->setTooltip((*customModBadge)->tooltip.string);
                continue;
            }
            this->emplace<ImageElement>(
                    Image::fromPixmap(app->resources->twitch.moderator),
                    MessageElementFlag::BadgeChannelAuthority)
                ->setTooltip("Twitch Channel Moderator");
        }
        else if (badge == "broadcaster/1")
        {
            this->emplace<ImageElement>(
                    Image::fromPixmap(app->resources->twitch.broadcaster),
                    MessageElementFlag::BadgeChannelAuthority)
                ->setTooltip("Twitch Broadcaster");
        }
        else if (badge == "turbo/1")
        {
            this->emplace<ImageElement>(
                    Image::fromPixmap(app->resources->twitch.turbo),
                    MessageElementFlag::BadgeVanity)
                ->setTooltip("Twitch Turbo Subscriber");
        }
        else if (badge == "premium/1")
        {
            this->emplace<ImageElement>(
                    Image::fromPixmap(app->resources->twitch.prime),
                    MessageElementFlag::BadgeVanity)
                ->setTooltip("Twitch Prime Subscriber");
        }
        else if (badge.startsWith("partner/"))
        {
            int index = badge.midRef(8).toInt();
            switch (index)
            {
                case 1:
                {
                    this->emplace<ImageElement>(
                            Image::fromPixmap(app->resources->twitch.verified,
                                              0.25),
                            MessageElementFlag::BadgeVanity)
                        ->setTooltip("Twitch Verified");
                }
                break;
                default:
                {
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
                this->emplace<EmoteElement>(
                        badgeEmote.get(), MessageElementFlag::BadgeSubscription)
                    ->setTooltip((*badgeEmote)->tooltip.string);
                continue;
            }

            // use default subscriber badge if custom one not found
            this->emplace<ImageElement>(
                    Image::fromPixmap(app->resources->twitch.subscriber, 0.25),
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
                this->emplace<EmoteElement>(badgeEmote.get(),
                                            MessageElementFlag::BadgeVanity)
                    ->setTooltip((*badgeEmote)->tooltip.string);
                continue;
            }
            if (auto badge = this->twitchChannel->globalTwitchBadges().badge(
                    splits[0], splits[1]))
            {
                this->emplace<EmoteElement>(badge.get(),
                                            MessageElementFlag::BadgeVanity)
                    ->setTooltip((*badge)->tooltip.string);
                continue;
            }
        }
    }
}

void TwitchMessageBuilder::appendChatterinoBadges()
{
    auto chatterinoBadgePtr =
        getApp()->chatterinoBadges->getBadge({this->userName});
    if (chatterinoBadgePtr)
    {
        this->emplace<EmoteElement>(*chatterinoBadgePtr,
                                    MessageElementFlag::BadgeChatterino);
    }
}

Outcome TwitchMessageBuilder::tryParseCheermote(const QString &string)
{
    // auto app = getApp();
    //// Try to parse custom cheermotes
    // const auto &channelResources = app->resources->channels[this->roomID_];
    // if (channelResources.loaded) {
    //    for (const auto &cheermoteSet : channelResources.cheermoteSets) {
    //        auto match = cheermoteSet.regex.match(string);
    //        if (!match.hasMatch()) {
    //            continue;
    //        }
    //        QString amount = match.captured(1);
    //        bool ok = false;
    //        int numBits = amount.toInt(&ok);
    //        if (!ok) {
    //            Log("Error parsing bit amount in tryParseCheermote");
    //            return Failure;
    //        }

    //        auto savedIt = cheermoteSet.cheermotes.end();

    //        // Fetch cheermote that matches our numBits
    //        for (auto it = cheermoteSet.cheermotes.begin(); it !=
    //        cheermoteSet.cheermotes.end();
    //             ++it) {
    //            if (numBits >= it->minBits) {
    //                savedIt = it;
    //            } else {
    //                break;
    //            }
    //        }

    //        if (savedIt == cheermoteSet.cheermotes.end()) {
    //            Log("Error getting a cheermote from a cheermote set for the
    //            bit amount {}",
    //                numBits);
    //            return Failure;
    //        }

    //        const auto &cheermote = *savedIt;

    //        this->emplace<EmoteElement>(cheermote.animatedEmote,
    //        MessageElementFlag::BitsAnimated);
    //        this->emplace<TextElement>(amount, MessageElementFlag::Text,
    //        cheermote.color);

    //        return Success;
    //    }
    //}

    return Failure;
}
}  // namespace chatterino
