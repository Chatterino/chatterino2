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

namespace chatterino {

TwitchMessageBuilder::TwitchMessageBuilder(Channel *_channel,
                                           const Communi::IrcPrivateMessage *_ircMessage,
                                           const MessageParseArgs &_args)
    : channel(_channel)
    , twitchChannel(dynamic_cast<TwitchChannel *>(_channel))
    , ircMessage(_ircMessage)
    , args(_args)
    , tags(this->ircMessage->tags())
    , originalMessage_(_ircMessage->content())
    , action_(_ircMessage->isAction())
{
    auto app = getApp();
    this->usernameColor_ = app->themes->messages.textColors.system;
}

TwitchMessageBuilder::TwitchMessageBuilder(Channel *_channel,
                                           const Communi::IrcMessage *_ircMessage,
                                           const MessageParseArgs &_args, QString content,
                                           bool isAction)
    : channel(_channel)
    , twitchChannel(dynamic_cast<TwitchChannel *>(_channel))
    , ircMessage(_ircMessage)
    , args(_args)
    , tags(this->ircMessage->tags())
    , originalMessage_(content)
    , action_(isAction)
{
    auto app = getApp();
    this->usernameColor_ = app->themes->messages.textColors.system;
}

bool TwitchMessageBuilder::isIgnored() const
{
    auto app = getApp();

    // TODO(pajlada): Do we need to check if the phrase is valid first?
    for (const auto &phrase : app->ignores->phrases.getVector()) {
        if (phrase.isMatch(this->originalMessage_)) {
            Log("Blocking message because it contains ignored phrase {}", phrase.getPattern());
            return true;
        }
    }

    if (app->settings->enableTwitchIgnoredUsers && this->tags.contains("user-id")) {
        auto sourceUserID = this->tags.value("user-id").toString();

        for (const auto &user : app->accounts->twitch.getCurrent()->getIgnores()) {
            if (sourceUserID == user.id) {
                Log("Blocking message because it's from blocked user {}", user.name);
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

    if (this->userName == this->channel->name) {
        this->senderIsBroadcaster = true;
    }

    //#ifdef XD
    //    if (this->originalMessage.length() > 100) {
    //        this->message->flags |= Message::Collapsed;
    //        this->emplace<EmoteElement>(getApp()->resources->badgeCollapsed,
    //        MessageElement::Collapsed);
    //    }
    //#endif
    this->message_->flags |= Message::Collapsed;

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
    if (/*app->settings->enableHighlights &&*/ !isPastMsg) {
        this->parseHighlights();
    }

    QString bits;
    auto iterator = this->tags.find("bits");
    if (iterator != this->tags.end()) {
        bits = iterator.value().toString();
    }

    // twitch emotes
    std::vector<std::pair<long, EmoteData>> twitchEmotes;

    iterator = this->tags.find("emotes");
    if (iterator != this->tags.end()) {
        QStringList emoteString = iterator.value().toString().split('/');

        for (QString emote : emoteString) {
            this->appendTwitchEmote(ircMessage, emote, twitchEmotes);
        }

        std::sort(twitchEmotes.begin(), twitchEmotes.end(),
                  [](const auto &a, const auto &b) { return a.first < b.first; });
    }

    auto currentTwitchEmote = twitchEmotes.begin();

    // words

    QStringList splits = this->originalMessage_.split(' ');

    long int i = 0;

    for (QString split : splits) {
        MessageColor textColor =
            this->action_ ? MessageColor(this->usernameColor_) : MessageColor(MessageColor::Text);

        // twitch emote
        if (currentTwitchEmote != twitchEmotes.end() && currentTwitchEmote->first == i) {
            auto emoteImage = currentTwitchEmote->second;
            this->emplace<EmoteElement>(emoteImage, MessageElement::TwitchEmote);

            i += split.length() + 1;
            currentTwitchEmote = std::next(currentTwitchEmote);

            continue;
        }

        // split words
        std::vector<std::tuple<EmoteData, QString>> parsed;

        // Parse emojis and take all non-emojis and put them in parsed as full text-words
        app->emotes->emojis.parse(parsed, split);

        for (const auto &tuple : parsed) {
            const EmoteData &emoteData = std::get<0>(tuple);

            if (!emoteData.isValid()) {  // is text
                QString string = std::get<1>(tuple);

                if (!bits.isEmpty() && this->tryParseCheermote(string)) {
                    // This string was parsed as a cheermote
                    continue;
                }

                // TODO: Implement ignored emotes
                // Format of ignored emotes:
                // Emote name: "forsenPuke" - if string in ignoredEmotes
                // Will match emote regardless of source (i.e. bttv, ffz)
                // Emote source + name: "bttv:nyanPls"
                if (this->tryAppendEmote(string)) {
                    // Successfully appended an emote
                    continue;
                }

                // Actually just text
                QString linkString = this->matchLink(string);
                auto fontStyle = FontStyle::ChatMedium;

                if (string.startsWith('@') && app->settings->usernameBold) {
                    fontStyle = FontStyle::ChatMediumBold;
                }

                Link link;

                if (linkString.isEmpty()) {
                    link = Link();
                } else {
                    if (app->settings->lowercaseLink) {
                        QRegularExpression httpRegex("\\bhttps?://",
                                                     QRegularExpression::CaseInsensitiveOption);
                        QRegularExpression ftpRegex("\\bftps?://",
                                                    QRegularExpression::CaseInsensitiveOption);
                        QRegularExpression getDomain("\\/\\/([^\\/]*)");
                        QString tempString = string;

                        if (!string.contains(httpRegex)) {
                            if (!string.contains(ftpRegex)) {
                                tempString.insert(0, "http://");
                            }
                        }
                        QString domain = getDomain.match(tempString).captured(1);
                        string.replace(domain, domain.toLower());
                    }
                    link = Link(Link::Url, linkString);
                    textColor = MessageColor(MessageColor::Link);
                }

                this->emplace<TextElement>(string, MessageElement::Text, textColor,
                                           fontStyle)  //
                    ->setLink(link);
            } else {  // is emoji
                this->emplace<EmoteElement>(emoteData, EmoteElement::EmojiAll);
            }
        }

        for (int j = 0; j < split.size(); j++) {
            i++;

            if (split.at(j).isHighSurrogate()) {
                j++;
            }
        }

        i++;
    }

    this->message_->searchText = this->userName + ": " + this->originalMessage_;

    return this->getMessage();
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

        if (this->twitchChannel->roomID.isEmpty()) {
            this->twitchChannel->roomID = this->roomID_;
        }
    }
}

void TwitchMessageBuilder::appendChannelName()
{
    QString channelName("#" + this->channel->name);
    Link link(Link::Url, this->channel->name + "\n" + this->messageID);

    this->emplace<TextElement>(channelName, MessageElement::ChannelName, MessageColor::System)  //
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
    //        this->userName = displayNameVariant.toString() + " (" + this->userName + ")";
    //    }

    this->message_->loginName = this->userName;
}

void TwitchMessageBuilder::appendUsername()
{
    auto app = getApp();

    QString username = this->userName;
    this->message_->loginName = username;
    QString localizedName;

    auto iterator = this->tags.find("display-name");
    if (iterator != this->tags.end()) {
        QString displayName = parseTagString(iterator.value().toString()).trimmed();

        if (QString::compare(displayName, this->userName, Qt::CaseInsensitive) == 0) {
            username = displayName;

            this->message_->displayName = displayName;
        } else {
            localizedName = displayName;

            this->message_->displayName = username;
            this->message_->localizedName = displayName;
        }
    }

    bool hasLocalizedName = !localizedName.isEmpty();

    // The full string that will be rendered in the chat widget
    QString usernameText;

    pajlada::Settings::Setting<int> usernameDisplayMode(
        "/appearance/messages/usernameDisplayMode", UsernameDisplayMode::UsernameAndLocalizedName);

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
        // userDisplayString += IrcManager::getInstance().getUser().getUserName();
    } else if (this->args.isReceivedWhisper) {
        // Sender username
        this->emplace<TextElement>(usernameText, MessageElement::Text, this->usernameColor_,
                                   FontStyle::ChatMediumBold)
            ->setLink({Link::UserInfo, this->userName});

        auto currentUser = app->accounts->twitch.getCurrent();

        // Separator
        this->emplace<TextElement>("->", MessageElement::Text,
                                   app->themes->messages.textColors.system, FontStyle::ChatMedium);

        QColor selfColor = currentUser->color;
        if (!selfColor.isValid()) {
            selfColor = app->themes->messages.textColors.system;
        }

        // Your own username
        this->emplace<TextElement>(currentUser->getUserName() + ":", MessageElement::Text,
                                   selfColor, FontStyle::ChatMediumBold);
    } else {
        if (!this->action_) {
            usernameText += ":";
        }

        this->emplace<TextElement>(usernameText, MessageElement::Text, this->usernameColor_,
                                   FontStyle::ChatMediumBold)
            ->setLink({Link::UserInfo, this->userName});
    }
}

void TwitchMessageBuilder::parseHighlights()
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
        highlightSoundUrl = QUrl(app->settings->pathHighlightSound.getValue());
    } else {
        highlightSoundUrl = QUrl("qrc:/sounds/ping2.wav");
    }

    if (currentPlayerUrl != highlightSoundUrl) {
        player->setMedia(highlightSoundUrl);

        currentPlayerUrl = highlightSoundUrl;
    }

    // TODO: This vector should only be rebuilt upon highlights being changed
    // fourtf: should be implemented in the HighlightsController
    std::vector<HighlightPhrase> activeHighlights = app->highlights->phrases.getVector();
    std::vector<HighlightPhrase> userHighlights = app->highlights->highlightedUsers.getVector();

    if (app->settings->enableHighlightsSelf && currentUsername.size() > 0) {
        HighlightPhrase selfHighlight(currentUsername, app->settings->enableHighlightTaskbar,
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
                    // Break if no further action can be taken from other highlights
                    // This might change if highlights can have custom colors/sounds/actions
                    break;
                }
            }
        }
        for (const HighlightPhrase &userHighlight : userHighlights) {
            if (userHighlight.isMatch(this->ircMessage->nick())) {
                Log("Highlight because user {} sent a message", this->ircMessage->nick());
                doHighlight = true;

                if (userHighlight.getAlert()) {
                    doAlert = true;
                }

                if (userHighlight.getSound()) {
                    playSound = true;
                }

                if (playSound && doAlert) {
                    // Break if no further action can be taken from other usernames
                    // Mostly used for regex stuff
                    break;
                }
            }
        }

        this->setHighlight(doHighlight);

        if (playSound && (!hasFocus || app->settings->highlightAlwaysPlaySound)) {
            player->play();
        }

        if (doAlert) {
            QApplication::alert(getApp()->windows->getMainWindow().window(), 2500);
        }

        if (doHighlight) {
            this->message_->flags |= Message::Highlighted;
        }
    }
}

void TwitchMessageBuilder::appendTwitchEmote(const Communi::IrcMessage *ircMessage,
                                             const QString &emote,
                                             std::vector<std::pair<long int, EmoteData>> &vec)
{
    auto app = getApp();
    if (!emote.contains(':')) {
        return;
    }

    QStringList parameters = emote.split(':');

    if (parameters.length() < 2) {
        return;
    }

    const auto &id = parameters.at(0);

    QStringList occurences = parameters.at(1).split(',');

    for (QString occurence : occurences) {
        QStringList coords = occurence.split('-');

        if (coords.length() < 2) {
            return;
        }

        int start = coords.at(0).toInt();
        int end = coords.at(1).toInt();

        if (start >= end || start < 0 || end > this->originalMessage_.length()) {
            return;
        }

        QString name = this->originalMessage_.mid(start, end - start + 1);

        vec.push_back(
            std::pair<long int, EmoteData>(start, app->emotes->twitch.getEmoteById(id, name)));
    }
}

bool TwitchMessageBuilder::tryAppendEmote(QString &emoteString)
{
    auto app = getApp();
    EmoteData emoteData;

    auto appendEmote = [&](MessageElement::Flags flags) {
        this->emplace<EmoteElement>(emoteData, flags);
        return true;
    };

    if (app->emotes->bttv.globalEmotes.tryGet(emoteString, emoteData)) {
        // BTTV Global Emote
        return appendEmote(MessageElement::BttvEmote);
    } else if (this->twitchChannel != nullptr &&
               this->twitchChannel->bttvChannelEmotes->tryGet(emoteString, emoteData)) {
        // BTTV Channel Emote
        return appendEmote(MessageElement::BttvEmote);
    } else if (app->emotes->ffz.globalEmotes.tryGet(emoteString, emoteData)) {
        // FFZ Global Emote
        return appendEmote(MessageElement::FfzEmote);
    } else if (this->twitchChannel != nullptr &&
               this->twitchChannel->ffzChannelEmotes->tryGet(emoteString, emoteData)) {
        // FFZ Channel Emote
        return appendEmote(MessageElement::FfzEmote);
    }

    return false;
}

// fourtf: this is ugly
//		   maybe put the individual badges into a map instead of this mess
void TwitchMessageBuilder::appendTwitchBadges()
{
    auto app = getApp();

    const auto &channelResources = app->resources->channels[this->roomID_];

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
            if (!app->resources->dynamicBadgesLoaded) {
                // Do nothing
                continue;
            }

            QString cheerAmountQS = badge.mid(5);
            std::string versionKey = cheerAmountQS.toStdString();
            QString tooltip = QString("Twitch cheer ") + cheerAmountQS;

            // Try to fetch channel-specific bit badge
            try {
                const auto &badge = channelResources.badgeSets.at("bits").versions.at(versionKey);
                this->emplace<ImageElement>(badge.badgeImage1x, MessageElement::BadgeVanity)
                    ->setTooltip(tooltip);
                continue;
            } catch (const std::out_of_range &) {
                // Channel does not contain a special bit badge for this version
            }

            // Use default bit badge
            try {
                const auto &badge = app->resources->badgeSets.at("bits").versions.at(versionKey);
                this->emplace<ImageElement>(badge.badgeImage1x, MessageElement::BadgeVanity)
                    ->setTooltip(tooltip);
            } catch (const std::out_of_range &) {
                Log("No default bit badge for version {} found", versionKey);
                continue;
            }
        } else if (badge == "staff/1") {
            this->emplace<ImageElement>(app->resources->badgeStaff,
                                        MessageElement::BadgeGlobalAuthority)
                ->setTooltip("Twitch Staff");
        } else if (badge == "admin/1") {
            this->emplace<ImageElement>(app->resources->badgeAdmin,
                                        MessageElement::BadgeGlobalAuthority)
                ->setTooltip("Twitch Admin");
        } else if (badge == "global_mod/1") {
            this->emplace<ImageElement>(app->resources->badgeGlobalModerator,
                                        MessageElement::BadgeGlobalAuthority)
                ->setTooltip("Twitch Global Moderator");
        } else if (badge == "moderator/1") {
            // TODO: Implement custom FFZ moderator badge
            this->emplace<ImageElement>(app->resources->badgeModerator,
                                        MessageElement::BadgeChannelAuthority)
                ->setTooltip("Twitch Channel Moderator");
        } else if (badge == "turbo/1") {
            this->emplace<ImageElement>(app->resources->badgeTurbo,
                                        MessageElement::BadgeGlobalAuthority)
                ->setTooltip("Twitch Turbo Subscriber");
        } else if (badge == "broadcaster/1") {
            this->emplace<ImageElement>(app->resources->badgeBroadcaster,
                                        MessageElement::BadgeChannelAuthority)
                ->setTooltip("Twitch Broadcaster");
        } else if (badge == "premium/1") {
            this->emplace<ImageElement>(app->resources->badgePremium, MessageElement::BadgeVanity)
                ->setTooltip("Twitch Prime Subscriber");
        } else if (badge.startsWith("partner/")) {
            int index = badge.midRef(8).toInt();
            switch (index) {
                case 1: {
                    this->emplace<ImageElement>(app->resources->badgeVerified,
                                                MessageElement::BadgeVanity)
                        ->setTooltip("Twitch Verified");
                } break;
                default: {
                    printf("[TwitchMessageBuilder] Unhandled partner badge index: %d\n", index);
                } break;
            }
        } else if (badge.startsWith("subscriber/")) {
            if (channelResources.loaded == false) {
                // qDebug() << "Channel resources are not loaded, can't add the subscriber badge";
                continue;
            }

            auto badgeSetIt = channelResources.badgeSets.find("subscriber");
            if (badgeSetIt == channelResources.badgeSets.end()) {
                // Fall back to default badge
                this->emplace<ImageElement>(app->resources->badgeSubscriber,
                                            MessageElement::BadgeSubscription)
                    ->setTooltip("Twitch Subscriber");
                continue;
            }

            const auto &badgeSet = badgeSetIt->second;

            std::string versionKey = badge.mid(11).toStdString();

            auto badgeVersionIt = badgeSet.versions.find(versionKey);

            if (badgeVersionIt == badgeSet.versions.end()) {
                // Fall back to default badge
                this->emplace<ImageElement>(app->resources->badgeSubscriber,
                                            MessageElement::BadgeSubscription)
                    ->setTooltip("Twitch Subscriber");
                continue;
            }

            auto &badgeVersion = badgeVersionIt->second;

            this->emplace<ImageElement>(badgeVersion.badgeImage1x,
                                        MessageElement::BadgeSubscription)
                ->setTooltip("Twitch " + QString::fromStdString(badgeVersion.title));
        } else {
            if (!app->resources->dynamicBadgesLoaded) {
                // Do nothing
                continue;
            }

            QStringList parts = badge.split('/');

            if (parts.length() != 2) {
                qDebug() << "Bad number of parts: " << parts.length() << " in " << parts;
                continue;
            }

            MessageElement::Flags badgeType = MessageElement::Flags::BadgeVanity;

            std::string badgeSetKey = parts[0].toStdString();
            std::string versionKey = parts[1].toStdString();

            try {
                auto &badgeSet = app->resources->badgeSets.at(badgeSetKey);

                try {
                    auto &badgeVersion = badgeSet.versions.at(versionKey);

                    this->emplace<ImageElement>(badgeVersion.badgeImage1x, badgeType)
                        ->setTooltip("Twitch " + QString::fromStdString(badgeVersion.title));
                } catch (const std::exception &e) {
                    qDebug() << "Exception caught:" << e.what()
                             << "when trying to fetch badge version " << versionKey.c_str();
                }
            } catch (const std::exception &e) {
                qDebug() << "No badge set with key" << badgeSetKey.c_str()
                         << ". Exception: " << e.what();
            }
        }
    }
}

void TwitchMessageBuilder::appendChatterinoBadges()
{
    auto app = getApp();

    auto &badges = app->resources->chatterinoBadges;
    auto it = badges.find(this->userName.toStdString());

    if (it == badges.end()) {
        return;
    }

    const auto badge = it->second;

    this->emplace<ImageElement>(badge->image, MessageElement::BadgeChatterino)
        ->setTooltip(QString::fromStdString(badge->tooltip));
}

bool TwitchMessageBuilder::tryParseCheermote(const QString &string)
{
    auto app = getApp();
    // Try to parse custom cheermotes
    const auto &channelResources = app->resources->channels[this->roomID_];
    if (channelResources.loaded) {
        for (const auto &cheermoteSet : channelResources.cheermoteSets) {
            auto match = cheermoteSet.regex.match(string);
            if (!match.hasMatch()) {
                continue;
            }
            QString amount = match.captured(1);
            bool ok = false;
            int numBits = amount.toInt(&ok);
            if (!ok) {
                Log("Error parsing bit amount in tryParseCheermote");
                return false;
            }

            auto savedIt = cheermoteSet.cheermotes.end();

            // Fetch cheermote that matches our numBits
            for (auto it = cheermoteSet.cheermotes.begin(); it != cheermoteSet.cheermotes.end();
                 ++it) {
                if (numBits >= it->minBits) {
                    savedIt = it;
                } else {
                    break;
                }
            }

            if (savedIt == cheermoteSet.cheermotes.end()) {
                Log("Error getting a cheermote from a cheermote set for the bit amount {}",
                    numBits);
                return false;
            }

            const auto &cheermote = *savedIt;

            this->emplace<EmoteElement>(cheermote.emoteDataAnimated, EmoteElement::BitsAnimated);
            this->emplace<TextElement>(amount, EmoteElement::Text, cheermote.color);

            return true;
        }
    }

    return false;
}
}  // namespace chatterino
