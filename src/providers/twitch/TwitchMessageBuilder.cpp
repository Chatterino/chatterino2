#include "providers/twitch/TwitchMessageBuilder.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/ignores/IgnoreController.hpp"
#include "debug/Log.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/IrcHelpers.hpp"

#include <QApplication>
#include <QDebug>
#include <QMediaPlayer>
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
    for (const auto &phrase : app->ignores->phrases.getVector()) {
        if (phrase.isMatch(this->originalMessage_)) {
            Log("Blocking message because it contains ignored phrase {}",
                phrase.getPattern());
            return true;
        }
    }

    if (app->settings->enableTwitchIgnoredUsers &&
        this->tags.contains("user-id")) {
        auto sourceUserID = this->tags.value("user-id").toString();

        for (const auto &user :
             app->accounts->twitch.getCurrent()->getIgnores()) {
            if (sourceUserID == user.id) {
                Log("Blocking message because it's from blocked user {}",
                    user.name);
                return true;
            }
        }
    }

    return false;
}

MessagePtr TwitchMessageBuilder::build()
{
    auto app = getApp();

    // PARSING
    this->parseUsername();

    if (this->userName == this->channel->getName()) {
        this->senderIsBroadcaster = true;
    }

    //#ifdef XD
    //    if (this->originalMessage.length() > 100) {
    //        this->message->flags.has(MessageFlag::Collapsed);
    //        this->emplace<EmoteElement>(getApp()->resources->badgeCollapsed,
    //        MessageElementFlag::Collapsed);
    //    }
    //#endif
    this->message().flags.has(MessageFlag::Collapsed);

    // PARSING
    this->parseMessageID();

    this->parseRoomID();

    this->appendChannelName();

    // timestamp
    bool isPastMsg = this->tags.contains("historical");
    if (isPastMsg) {
        // This may be architecture dependent(datatype)
        qint64 ts = this->tags.value("tmi-sent-ts").toLongLong();
        QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(ts);
        this->emplace<TimestampElement>(dateTime.time());
    } else {
        this->emplace<TimestampElement>();
    }

    bool addModerationElement = true;
    if (this->senderIsBroadcaster) {
        addModerationElement = false;
    } else {
        bool hasUserType = this->tags.contains("user-type");
        if (hasUserType) {
            QString userType = this->tags.value("user-type").toString();

            if (userType == "mod") {
                if (!args.isStaffOrBroadcaster) {
                    addModerationElement = false;
                }
            }
        }
    }

    if (addModerationElement) {
        this->emplace<TwitchModerationElement>();
    }

    this->appendTwitchBadges();

    this->appendChatterinoBadges();

    this->appendUsername();

    // highlights
    this->parseHighlights(isPastMsg);

    //    QString bits;
    auto iterator = this->tags.find("bits");
    if (iterator != this->tags.end()) {
        this->hasBits_ = true;
        //        bits = iterator.value().toString();
    }

    // twitch emotes
    std::vector<std::pair<int, EmotePtr>> twitchEmotes;

    iterator = this->tags.find("emotes");
    if (iterator != this->tags.end()) {
        QStringList emoteString = iterator.value().toString().split('/');

        for (QString emote : emoteString) {
            this->appendTwitchEmote(ircMessage, emote, twitchEmotes);
        }

        std::sort(
            twitchEmotes.begin(), twitchEmotes.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });
    }

    // words
    QStringList splits = this->originalMessage_.split(' ');

    this->addWords(splits, twitchEmotes);

    this->message().searchText = this->userName + ": " + this->originalMessage_;

    return this->release();
}

void TwitchMessageBuilder::addWords(
    const QStringList &words,
    const std::vector<std::pair<int, EmotePtr>> &twitchEmotes)
{
    auto i = int();
    auto currentTwitchEmote = twitchEmotes.begin();

    for (const auto &word : words) {
        // check if it's a twitch emote twitch emote
        if (currentTwitchEmote != twitchEmotes.end() &&
            currentTwitchEmote->first == i) {
            auto emoteImage = currentTwitchEmote->second;
            this->emplace<EmoteElement>(emoteImage,
                                        MessageElementFlag::TwitchEmote);

            i += word.length() + 1;
            currentTwitchEmote++;

            continue;
        }

        // split words
        for (auto &variant : getApp()->emotes->emojis.parse(word)) {
            boost::apply_visitor(/*overloaded{[&](EmotePtr arg) {
                                    this->addTextOrEmoji(arg); },
                                    [&](const QString &arg) {
                                    this->addTextOrEmoji(arg); }}*/
                                 [&](auto &&arg) { this->addTextOrEmoji(arg); },
                                 variant);
        }

        for (int j = 0; j < word.size(); j++) {
            i++;

            if (word.at(j).isHighSurrogate()) {
                j++;
            }
        }

        i++;
    }
}

void TwitchMessageBuilder::addTextOrEmoji(EmotePtr emote)
{
    this->emplace<EmoteElement>(emote, MessageElementFlag::EmojiAll);
}

void TwitchMessageBuilder::addTextOrEmoji(const QString &string_)
{
    auto string = QString(string_);

    if (this->hasBits_ && this->tryParseCheermote(string)) {
        // This string was parsed as a cheermote
        return;
    }

    // TODO: Implement ignored emotes
    // Format of ignored emotes:
    // Emote name: "forsenPuke" - if string in ignoredEmotes
    // Will match emote regardless of source (i.e. bttv, ffz)
    // Emote source + name: "bttv:nyanPls"
    if (this->tryAppendEmote({string})) {
        // Successfully appended an emote
        return;
    }

    // Actually just text
    auto linkString = this->matchLink(string);
    auto link = Link();
    auto textColor = this->action_ ? MessageColor(this->usernameColor_)
                                   : MessageColor(MessageColor::Text);

    if (linkString.isEmpty()) {
        if (string.startsWith('@')) {
            this->emplace<TextElement>(string, MessageElementFlag::BoldUsername,
                                       textColor, FontStyle::ChatMediumBold);
            this->emplace<TextElement>(
                string, MessageElementFlag::NonBoldUsername, textColor);
        } else {
            this->emplace<TextElement>(string, MessageElementFlag::Text,
                                       textColor);
        }
    } else {
        static QRegularExpression domainRegex(
            R"(^(?:(?:ftp|http)s?:\/\/)?([^\/:]+)(?:\/.*)?$)",
            QRegularExpression::CaseInsensitiveOption);

        QString lowercaseLinkString;
        auto match = domainRegex.match(string);
        if (match.isValid()) {
            lowercaseLinkString = string.mid(0, match.capturedStart(1)) +
                                  match.captured(1).toLower() +
                                  string.mid(match.capturedEnd(1));
        } else {
            lowercaseLinkString = string;
        }
        link = Link(Link::Url, linkString);

        textColor = MessageColor(MessageColor::Link);
        this->emplace<TextElement>(lowercaseLinkString,
                                   MessageElementFlag::LowercaseLink, textColor)
            ->setLink(link);
        this->emplace<TextElement>(string, MessageElementFlag::OriginalLink,
                                   textColor)
            ->setLink(link);
    }

    // if (!linkString.isEmpty()) {
    //    if (getApp()->settings->lowercaseLink) {
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

    if (iterator != this->tags.end()) {
        this->messageID = iterator.value().toString();
    }
}

void TwitchMessageBuilder::parseRoomID()
{
    if (this->twitchChannel == nullptr) {
        return;
    }

    auto iterator = this->tags.find("room-id");

    if (iterator != std::end(this->tags)) {
        this->roomID_ = iterator.value().toString();

        if (this->twitchChannel->getRoomId().isEmpty()) {
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
    if (iterator != this->tags.end()) {
        this->usernameColor_ = QColor(iterator.value().toString());
    }

    // username
    this->userName = this->ircMessage->nick();

    if (this->userName.isEmpty() || this->args.trimSubscriberUsername) {
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
    if (iterator != this->tags.end()) {
        QString displayName =
            parseTagString(iterator.value().toString()).trimmed();

        if (QString::compare(displayName, this->userName,
                             Qt::CaseInsensitive) == 0) {
            username = displayName;

            this->message().displayName = displayName;
        } else {
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

    switch (usernameDisplayMode.getValue()) {
        case UsernameDisplayMode::Username: {
            usernameText = username;
        } break;

        case UsernameDisplayMode::LocalizedName: {
            if (hasLocalizedName) {
                usernameText = localizedName;
            } else {
                usernameText = username;
            }
        } break;

        default:
        case UsernameDisplayMode::UsernameAndLocalizedName: {
            if (hasLocalizedName) {
                usernameText = username + "(" + localizedName + ")";
            } else {
                usernameText = username;
            }
        } break;
    }

    if (this->args.isSentWhisper) {
        // TODO(pajlada): Re-implement
        // userDisplayString +=
        // IrcManager::getInstance().getUser().getUserName();
    } else if (this->args.isReceivedWhisper) {
        // Sender username
        this->emplace<TextElement>(usernameText, MessageElementFlag::Text,
                                   this->usernameColor_,
                                   FontStyle::ChatMediumBold)
            ->setLink({Link::UserInfo, this->userName});

        auto currentUser = app->accounts->twitch.getCurrent();

        // Separator
        this->emplace<TextElement>("->", MessageElementFlag::Text,
                                   app->themes->messages.textColors.system,
                                   FontStyle::ChatMedium);

        QColor selfColor = currentUser->color;
        if (!selfColor.isValid()) {
            selfColor = app->themes->messages.textColors.system;
        }

        // Your own username
        this->emplace<TextElement>(currentUser->getUserName() + ":",
                                   MessageElementFlag::Text, selfColor,
                                   FontStyle::ChatMediumBold);
    } else {
        if (!this->action_) {
            usernameText += ":";
        }

        this->emplace<TextElement>(usernameText, MessageElementFlag::Text,
                                   this->usernameColor_,
                                   FontStyle::ChatMediumBold)
            ->setLink({Link::UserInfo, this->userName});
    }
}

void TwitchMessageBuilder::parseHighlights(bool isPastMsg)
{
    static auto player = new QMediaPlayer;
    static QUrl currentPlayerUrl;

    auto app = getApp();

    auto currentUser = app->accounts->twitch.getCurrent();

    QString currentUsername = currentUser->getUserName();

    if (this->ircMessage->nick() == currentUsername) {
        currentUser->color = this->usernameColor_;
        // Do nothing. Highlights cannot be triggered by yourself
        return;
    }

    // update the media player url if necessary
    QUrl highlightSoundUrl;
    if (app->settings->customHighlightSound) {
        highlightSoundUrl =
            QUrl::fromLocalFile(app->settings->pathHighlightSound.getValue());
    } else {
        highlightSoundUrl = QUrl("qrc:/sounds/ping2.wav");
    }

    if (currentPlayerUrl != highlightSoundUrl) {
        player->setMedia(highlightSoundUrl);

        currentPlayerUrl = highlightSoundUrl;
    }

    // TODO: This vector should only be rebuilt upon highlights being changed
    // fourtf: should be implemented in the HighlightsController
    std::vector<HighlightPhrase> activeHighlights =
        app->highlights->phrases.getVector();
    std::vector<HighlightPhrase> userHighlights =
        app->highlights->highlightedUsers.getVector();

    if (app->settings->enableHighlightsSelf && currentUsername.size() > 0) {
        HighlightPhrase selfHighlight(
            currentUsername, app->settings->enableHighlightTaskbar,
            app->settings->enableHighlightSound, false);
        activeHighlights.emplace_back(std::move(selfHighlight));
    }

    bool doHighlight = false;
    bool playSound = false;
    bool doAlert = false;

    bool hasFocus = (QApplication::focusWidget() != nullptr);

    if (!app->highlights->blacklistContains(this->ircMessage->nick())) {
        for (const HighlightPhrase &highlight : activeHighlights) {
            if (highlight.isMatch(this->originalMessage_)) {
                Log("Highlight because {} matches {}", this->originalMessage_,
                    highlight.getPattern());
                doHighlight = true;

                if (highlight.getAlert()) {
                    doAlert = true;
                }

                if (highlight.getSound()) {
                    playSound = true;
                }

                if (playSound && doAlert) {
                    // Break if no further action can be taken from other
                    // highlights This might change if highlights can have
                    // custom colors/sounds/actions
                    break;
                }
            }
        }
        for (const HighlightPhrase &userHighlight : userHighlights) {
            if (userHighlight.isMatch(this->ircMessage->nick())) {
                Log("Highlight because user {} sent a message",
                    this->ircMessage->nick());
                doHighlight = true;

                if (userHighlight.getAlert()) {
                    doAlert = true;
                }

                if (userHighlight.getSound()) {
                    playSound = true;
                }

                if (playSound && doAlert) {
                    // Break if no further action can be taken from other
                    // usernames Mostly used for regex stuff
                    break;
                }
            }
        }

        this->message().flags.set(MessageFlag::Highlighted, doHighlight);

        if (!isPastMsg) {
            if (playSound &&
                (!hasFocus || app->settings->highlightAlwaysPlaySound)) {
                player->play();
            }

            if (doAlert) {
                QApplication::alert(getApp()->windows->getMainWindow().window(),
                                    2500);
            }
        }
    }
}

void TwitchMessageBuilder::appendTwitchEmote(
    const Communi::IrcMessage *ircMessage, const QString &emote,
    std::vector<std::pair<int, EmotePtr>> &vec)
{
    auto app = getApp();
    if (!emote.contains(':')) {
        return;
    }

    auto parameters = emote.split(':');

    if (parameters.length() < 2) {
        return;
    }

    auto id = EmoteId{parameters.at(0)};

    auto occurences = parameters.at(1).split(',');

    for (QString occurence : occurences) {
        auto coords = occurence.split('-');

        if (coords.length() < 2) {
            return;
        }

        auto start = coords.at(0).toInt();
        auto end = coords.at(1).toInt();

        if (start >= end || start < 0 ||
            end > this->originalMessage_.length()) {
            return;
        }

        auto name =
            EmoteName{this->originalMessage_.mid(start, end - start + 1)};

        vec.push_back(std::make_pair(
            start, app->emotes->twitch.getOrCreateEmote(id, name)));
    }
}

Outcome TwitchMessageBuilder::tryAppendEmote(const EmoteName &name)
{
    auto flags = MessageElementFlags();
    auto emote = boost::optional<EmotePtr>{};

    if ((emote = getApp()->emotes->bttv.getGlobalEmote(name))) {
        flags = MessageElementFlag::BttvEmote;
    } else if (twitchChannel &&
               (emote = this->twitchChannel->getBttvEmote(name))) {
        flags = MessageElementFlag::BttvEmote;
    } else if ((emote = getApp()->emotes->ffz.getGlobalEmote(name))) {
        flags = MessageElementFlag::FfzEmote;
    } else if (twitchChannel &&
               (emote = this->twitchChannel->getFfzEmote(name))) {
        flags = MessageElementFlag::FfzEmote;
    }

    if (emote) {
        this->emplace<EmoteElement>(emote.get(), flags);
        return Success;
    }

    return Failure;
}

// fourtf: this is ugly
//		   maybe put the individual badges into a map instead of this
// mess
void TwitchMessageBuilder::appendTwitchBadges()
{
    auto app = getApp();

    auto iterator = this->tags.find("badges");

    if (iterator == this->tags.end()) {
        // No badges in this message
        return;
    }

    QStringList badges = iterator.value().toString().split(',');

    for (QString badge : badges) {
        if (badge.isEmpty()) {
            continue;
        }

        if (badge.startsWith("bits/")) {
            // if (!app->resources->dynamicBadgesLoaded) {
            //     // Do nothing
            //     continue;
            // }

            QString cheerAmount = badge.mid(5);
            QString tooltip = QString("Twitch cheer ") + cheerAmount;

            // Try to fetch channel-specific bit badge
            try {
                if (twitchChannel)
                    if (const auto &badge = this->twitchChannel->getTwitchBadge(
                            "bits", cheerAmount)) {
                        this->emplace<EmoteElement>(
                                badge.get(), MessageElementFlag::BadgeVanity)
                            ->setTooltip(tooltip);
                        continue;
                    }
            } catch (const std::out_of_range &) {
                // Channel does not contain a special bit badge for this version
            }

            // Use default bit badge
            // try {
            //    const auto &badge =
            //    app->resources->badgeSets.at("bits").versions.at(cheerAmount);
            //    this->emplace<ImageElement>(badge.badgeImage1x,
            //    MessageElementFlag::BadgeVanity)
            //        ->setTooltip(tooltip);
            //} catch (const std::out_of_range &) {
            //    Log("No default bit badge for version {} found", cheerAmount);
            //    continue;
            //}
        } else if (badge == "staff/1") {
            this->emplace<ImageElement>(
                    Image::fromNonOwningPixmap(&app->resources->twitch.staff),
                    MessageElementFlag::BadgeGlobalAuthority)
                ->setTooltip("Twitch Staff");
        } else if (badge == "admin/1") {
            this->emplace<ImageElement>(
                    Image::fromNonOwningPixmap(&app->resources->twitch.admin),
                    MessageElementFlag::BadgeGlobalAuthority)
                ->setTooltip("Twitch Admin");
        } else if (badge == "global_mod/1") {
            this->emplace<ImageElement>(
                    Image::fromNonOwningPixmap(
                        &app->resources->twitch.globalmod),
                    MessageElementFlag::BadgeGlobalAuthority)
                ->setTooltip("Twitch Global Moderator");
        } else if (badge == "moderator/1") {
            // TODO: Implement custom FFZ moderator badge
            this->emplace<ImageElement>(
                    Image::fromNonOwningPixmap(
                        &app->resources->twitch.moderator),
                    MessageElementFlag::BadgeChannelAuthority)
                ->setTooltip("Twitch Channel Moderator");
        } else if (badge == "turbo/1") {
            this->emplace<ImageElement>(
                    Image::fromNonOwningPixmap(&app->resources->twitch.turbo),
                    MessageElementFlag::BadgeGlobalAuthority)
                ->setTooltip("Twitch Turbo Subscriber");
        } else if (badge == "broadcaster/1") {
            this->emplace<ImageElement>(
                    Image::fromNonOwningPixmap(
                        &app->resources->twitch.broadcaster),
                    MessageElementFlag::BadgeChannelAuthority)
                ->setTooltip("Twitch Broadcaster");
        } else if (badge == "premium/1") {
            this->emplace<ImageElement>(
                    Image::fromNonOwningPixmap(&app->resources->twitch.prime),
                    MessageElementFlag::BadgeVanity)
                ->setTooltip("Twitch Prime Subscriber");
        } else if (badge.startsWith("partner/")) {
            int index = badge.midRef(8).toInt();
            switch (index) {
                case 1: {
                    this->emplace<ImageElement>(
                            Image::fromNonOwningPixmap(
                                &app->resources->twitch.verified, 0.25),
                            MessageElementFlag::BadgeVanity)
                        ->setTooltip("Twitch Verified");
                } break;
                default: {
                    printf("[TwitchMessageBuilder] Unhandled partner badge "
                           "index: %d\n",
                           index);
                } break;
            }
        } else if (badge.startsWith("subscriber/")) {
            //            if (channelResources.loaded == false) {
            //                // qDebug() << "Channel resources are not loaded,
            //                can't add the subscriber
            //                // badge";
            //                continue;
            //            }

            // auto badgeSetIt = channelResources.badgeSets.find("subscriber");
            // if (badgeSetIt == channelResources.badgeSets.end()) {
            //    // Fall back to default badge
            //    this->emplace<ImageElement>(app->resources->badgeSubscriber,
            //                                MessageElementFlag::BadgeSubscription)
            //        ->setTooltip("Twitch Subscriber");
            //    continue;
            //}

            // const auto &badgeSet = badgeSetIt->second;

            // std::string versionKey = badge.mid(11).toStdString();

            // auto badgeVersionIt = badgeSet.versions.find(versionKey);

            // if (badgeVersionIt == badgeSet.versions.end()) {
            //    // Fall back to default badge
            //    this->emplace<ImageElement>(app->resources->badgeSubscriber,
            //                                MessageElementFlag::BadgeSubscription)
            //        ->setTooltip("Twitch Subscriber");
            //    continue;
            //}

            // auto &badgeVersion = badgeVersionIt->second;

            // this->emplace<ImageElement>(badgeVersion.badgeImage1x,
            //                            MessageElementFlag::BadgeSubscription)
            //    ->setTooltip("Twitch " +
            //    QString::fromStdString(badgeVersion.title));
        } else {
            // if (!app->resources->dynamicBadgesLoaded) {
            //    // Do nothing
            //    continue;
            //}

            // QStringList parts = badge.split('/');

            // if (parts.length() != 2) {
            //    qDebug() << "Bad number of parts: " << parts.length() << " in
            //    " << parts; continue;
            //}

            // MessageElementFlags badgeType =
            // MessageElementFlag::BadgeVanity;

            // std::string badgeSetKey = parts[0].toStdString();
            // std::string versionKey = parts[1].toStdString();

            // try {
            //    auto &badgeSet = app->resources->badgeSets.at(badgeSetKey);

            //    try {
            //        auto &badgeVersion = badgeSet.versions.at(versionKey);

            //        this->emplace<ImageElement>(badgeVersion.badgeImage1x,
            //        badgeType)
            //            ->setTooltip("Twitch " +
            //            QString::fromStdString(badgeVersion.title));
            //    } catch (const std::exception &e) {
            //        qDebug() << "Exception caught:" << e.what()
            //                 << "when trying to fetch badge version " <<
            //                 versionKey.c_str();
            //    }
            //} catch (const std::exception &e) {
            //    qDebug() << "No badge set with key" << badgeSetKey.c_str()
            //             << ". Exception: " << e.what();
            //}
        }
    }
}

void TwitchMessageBuilder::appendChatterinoBadges()
{
    //    auto app = getApp();

    //    auto &badges = app->resources->chatterinoBadges;
    //    auto it = badges.find(this->userName.toStdString());

    //    if (it == badges.end()) {
    //        return;
    //    }

    //    const auto badge = it->second;

    //    this->emplace<ImageElement>(badge->image,
    //    MessageElementFlag::BadgeChatterino)
    //        ->setTooltip(QString::fromStdString(badge->tooltip));
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
