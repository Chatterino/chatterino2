#include "twitch/twitchmessagebuilder.hpp"
#include "colorscheme.hpp"
#include "debug/log.hpp"
#include "emotemanager.hpp"
#include "ircmanager.hpp"
#include "resources.hpp"
#include "settingsmanager.hpp"
#include "windowmanager.hpp"

#include <QApplication>
#include <QDebug>
#include <QMediaPlayer>

using namespace chatterino::messages;

namespace chatterino {
namespace twitch {

TwitchMessageBuilder::TwitchMessageBuilder(TwitchChannel *_channel, Resources &_resources,
                                           EmoteManager &_emoteManager,
                                           WindowManager &_windowManager,
                                           const Communi::IrcPrivateMessage *_ircMessage,
                                           const messages::MessageParseArgs &_args)
    : channel(_channel)
    , twitchChannel(_channel)
    , resources(_resources)
    , windowManager(_windowManager)
    , colorScheme(this->windowManager.colorScheme)
    , emoteManager(_emoteManager)
    , ircMessage(_ircMessage)
    , args(_args)
    , tags(this->ircMessage->tags())
    , usernameColor(this->colorScheme.SystemMessageColor)
{
}

SharedMessage TwitchMessageBuilder::parse()
{
    SettingsManager &settings = SettingsManager::getInstance();

    this->originalMessage = this->ircMessage->content();

    this->parseUsername();

    // The timestamp is always appended to the builder
    // Whether or not will be rendered is decided/checked later
    this->appendTimestamp();

    this->parseMessageID();

    this->parseRoomID();

    this->appendModerationButtons();

    /*
    this->parseTwitchBadges();

    this->parseChatterinoBadges();
*/

    if (this->args.includeChannelName) {
        this->parseChannelName();
    }

    this->appendUsername();

    // highlights
    if (settings.enableHighlights.get()) {
        this->parseHighlights();
    }

    // bits
    QString bits = "";

    auto iterator = this->tags.find("bits");
    if (iterator != this->tags.end()) {
        bits = iterator.value().toString();
    }

    // twitch emotes
    std::vector<std::pair<long, Emote>> twitchEmotes;

    iterator = this->tags.find("emotes");
    if (iterator != this->tags.end()) {
        QStringList emoteString = iterator.value().toString().split('/');

        for (QString emote : emoteString) {
            this->appendTwitchEmote(ircMessage, emote, twitchEmotes, emoteManager);
        }

        struct {
            bool operator()(const std::pair<long, Emote> &lhs, const std::pair<long, Emote> &rhs)
            {
                return lhs.first < rhs.first;
            }
        } customLess;

        std::sort(twitchEmotes.begin(), twitchEmotes.end(), customLess);
    }

    auto currentTwitchEmote = twitchEmotes.begin();

    // words

    QStringList splits = this->originalMessage.split(' ');

    long int i = 0;

    for (QString split : splits) {
        MessageColor textColor = ircMessage->isAction() ? MessageColor(this->usernameColor)
                                                        : MessageColor(MessageColor::Text);

        // twitch emote
        if (currentTwitchEmote != twitchEmotes.end() && currentTwitchEmote->first == i) {
            this->appendWord(Word(currentTwitchEmote->second.url, Word::TwitchEmoteImage,
                                  currentTwitchEmote->second.name));
            /*this->appendWord(Word(currentTwitchEmote->second.url, Word::TwitchEmoteText,
               textColor, currentTwitchEmote->second.name));*/

            i += split.length() + 1;
            currentTwitchEmote = std::next(currentTwitchEmote);

            continue;
        }

        // split words
        std::vector<std::tuple<std::unique_ptr<Emote>, QString>> parsed;

        // Parse emojis and take all non-emojis and put them in parsed as full text-words
        emoteManager.parseEmojis(parsed, split);

        for (const std::tuple<std::unique_ptr<Emote>, QString> &tuple : parsed) {
            Emote *emote = std::get<0>(tuple).get();

            if (emote == nullptr) {  // is text
                QString string = std::get<1>(tuple);

                static QRegularExpression cheerRegex("cheer[1-9][0-9]*");

                // cheers
                if (!bits.isEmpty() && string.length() >= 6 && cheerRegex.match(string).isValid()) {
                    auto cheer = string.mid(5).toInt();

                    QString color;

                    QColor bitsColor;

                    if (cheer >= 10000) {
                        color = "red";
                        bitsColor = QColor::fromHslF(0, 1, 0.5);
                    } else if (cheer >= 5000) {
                        color = "blue";
                        bitsColor = QColor::fromHslF(0.61, 1, 0.4);
                    } else if (cheer >= 1000) {
                        color = "green";
                        bitsColor = QColor::fromHslF(0.5, 1, 0.5);
                    } else if (cheer >= 100) {
                        color = "purple";
                        bitsColor = QColor::fromHslF(0.8, 1, 0.5);
                    } else {
                        color = "gray";
                        bitsColor = QColor::fromHslF(0.5f, 0.5f, 0.5f);
                    }

                    QString bitsLinkAnimated =
                        QString("http://static-cdn.jtvnw.net/bits/dark/animated/" + color + "/1");
                    QString bitsLink =
                        QString("http://static-cdn.jtvnw.net/bits/dark/static/" + color + "/1");

                    this->appendWord(Word(bitsLinkAnimated, Word::BitsAnimated,
                                          QString("Twitch Cheer"),
                                          Link(Link::Url, QString("https://blog.twitch.tv/"
                                                                  "introducing-cheering-celebrate-"
                                                                  "together-da62af41fac6"))));
                    this->appendWord(Word(
                        bitsLink, Word::BitsStatic, QString("Twitch Cheer"),
                        Link(Link::Url,
                             QString("https://blog.twitch.tv/"
                                     "introducing-cheering-celebrate-together-da62af41fac6"))));

                    this->appendWord(Word(
                        QString("x" + string.mid(5)), Word::BitsAmount, MessageColor(bitsColor),
                        QString("Twitch Cheer"),
                        Link(Link::Url,
                             QString("https://blog.twitch.tv/"
                                     "introducing-cheering-celebrate-together-da62af41fac6"))));

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

                Link link;

                if (linkString.isEmpty()) {
                    link = Link();
                } else {
                    link = Link(Link::Url, linkString);
                    textColor = MessageColor(MessageColor::Link);
                }

                this->appendWord(Word(string, Word::Text, textColor, string, link));
            } else {  // is emoji
                static QString emojiTooltip("Emoji");

                this->appendWord(Word(emote->url, Word::EmojiImage, emojiTooltip));
                /*Word(emoteData.image->getName(), Word::EmojiText, textColor,
                     emoteData.image->getName(), emojiTooltip);*/
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
    auto iterator = this->tags.find("room-id");

    if (iterator != std::end(this->tags)) {
        this->roomID = iterator.value().toString().toStdString();

        if (this->twitchChannel->roomID.empty()) {
            this->twitchChannel->roomID = this->roomID;
        }
    }
}

void TwitchMessageBuilder::parseChannelName()
{
    QString channelName("#" + this->channel->name);
    this->appendWord(Word(channelName, Word::Misc, MessageColor(MessageColor::System),
                          QString(channelName),
                          Link(Link::Url, this->channel->name + "\n" + this->messageID)));
}

void TwitchMessageBuilder::parseUsername()
{
    auto iterator = this->tags.find("color");
    if (iterator != this->tags.end()) {
        this->usernameColor = QColor(iterator.value().toString());
    }

    // username
    this->userName = ircMessage->nick();

    if (this->userName.isEmpty()) {
        this->userName = this->tags.value(QLatin1String("login")).toString();
    }
}

void TwitchMessageBuilder::appendUsername()
{
    QString username = this->userName;
    QString localizedName;

    auto iterator = this->tags.find("display-name");
    if (iterator != this->tags.end()) {
        QString displayName = iterator.value().toString();

        if (QString::compare(displayName, this->userName, Qt::CaseInsensitive) == 0) {
            username = displayName;
        } else {
            localizedName = displayName;
        }
    }

    bool hasLocalizedName = !localizedName.isEmpty();

    // The full string that will be rendered in the chat widget
    QString usernameString;

    pajlada::Settings::Setting<int> usernameDisplayMode(
        "/appearance/messages/usernameDisplayMode", UsernameDisplayMode::UsernameAndLocalizedName);

    switch (usernameDisplayMode.getValue()) {
        case UsernameDisplayMode::Username: {
            usernameString = username;
        } break;

        case UsernameDisplayMode::LocalizedName: {
            if (hasLocalizedName) {
                usernameString = localizedName;
            } else {
                usernameString = username;
            }
        } break;

        default:
        case UsernameDisplayMode::UsernameAndLocalizedName: {
            if (hasLocalizedName) {
                usernameString = username + "(" + localizedName + ")";
            } else {
                usernameString = username;
            }
        } break;
    }

    if (this->args.isSentWhisper) {
        // TODO(pajlada): Re-implement
        // userDisplayString += IrcManager::getInstance().getUser().getUserName();
    }

    if (this->args.isReceivedWhisper) {
        // TODO(pajlada): Re-implement
        // userDisplayString += " -> " + IrcManager::getInstance().getUser().getUserName();
    }

    if (!ircMessage->isAction()) {
        usernameString += ": ";
    }

    this->appendWord(Word(usernameString, Word::Username, MessageColor(this->usernameColor),
                          usernameString, Link(Link::UserInfo, this->userName)));
}

void TwitchMessageBuilder::parseHighlights()
{
    static auto player = new QMediaPlayer;
    SettingsManager &settings = SettingsManager::getInstance();
    static pajlada::Settings::Setting<std::string> currentUser("/accounts/current");

    QString currentUsername = QString::fromStdString(currentUser.getValue());

    if (this->ircMessage->nick() == currentUsername) {
        // Do nothing. Highlights cannot be triggered by yourself
        return;
    }

    if (settings.customHighlightSound.get()) {
        player->setMedia(QUrl(settings.pathHighlightSound.get()));
    } else {
        player->setMedia(QUrl("qrc:/sounds/ping2.wav"));
    }

    struct Highlight {
        Highlight(const QString &_target, bool _sound, bool _alert)
            : target(_target)
            , sound(_sound)
            , alert(_alert)
        {
        }

        QString target;
        bool sound;
        bool alert;
    };

    QStringList blackList =
        settings.highlightUserBlacklist.get().split("\n", QString::SkipEmptyParts);

    // TODO: This vector should only be rebuilt upon highlights being changed
    std::vector<Highlight> activeHighlights;

    if (settings.enableHighlightsSelf.get() && currentUsername.size() > 0) {
        activeHighlights.emplace_back(currentUsername, settings.enableHighlightSound.get(),
                                      settings.enableHighlightTaskbar.get());
    }

    const auto &highlightProperties = settings.highlightProperties.get();

    for (auto it = highlightProperties.begin(); it != highlightProperties.end(); ++it) {
        auto properties = it.value();
        activeHighlights.emplace_back(it.key(), properties.first, properties.second);
    }

    bool doHighlight = false;
    bool playSound = false;
    bool doAlert = false;
    if (!blackList.contains(this->ircMessage->nick(), Qt::CaseInsensitive)) {
        for (const Highlight &highlight : activeHighlights) {
            if (this->originalMessage.contains(highlight.target, Qt::CaseInsensitive)) {
                debug::Log("Highlight because {} contains {}", this->originalMessage,
                           highlight.target);
                doHighlight = true;

                if (highlight.sound) {
                    playSound = true;
                }

                if (highlight.alert) {
                    doAlert = true;
                }

                if (playSound && doAlert) {
                    // Break if no further action can be taken from other highlights
                    // This might change if highlights can have custom colors/sounds/actions
                    break;
                }
            }
        }

        this->setHighlight(doHighlight);

        if (playSound) {
            player->play();
        }

        if (doAlert) {
            QApplication::alert(windowManager.getMainWindow().window(), 2500);
        }
    }
}

void TwitchMessageBuilder::appendModerationButtons()
{
    // mod buttons

    static QString buttonBanTooltip("Ban user");
    static QString buttonTimeoutTooltip("Timeout user");

    // TODO: Implement ban buttons
    /*
        this->appendWord(Word(this->resources.buttonBan->getUrl(), Word::ButtonBan, QString(),
                              buttonBanTooltip, Link(Link::UserBan, ircMessage->account())));
        this->appendWord(Word(this->resources.buttonTimeout->getUrl(), Word::ButtonTimeout,
       QString(), buttonTimeoutTooltip, Link(Link::UserTimeout, ircMessage->account())));
                              */
}

void TwitchMessageBuilder::appendTwitchEmote(const Communi::IrcPrivateMessage *ircMessage,
                                             const QString &emote,
                                             std::vector<std::pair<long, Emote>> &vec,
                                             EmoteManager &emoteManager)
{
    if (!emote.contains(':')) {
        return;
    }

    QStringList parameters = emote.split(':');

    if (parameters.length() < 2) {
        return;
    }

    long int id = std::stol(parameters.at(0).toStdString(), nullptr, 10);

    QStringList occurences = parameters.at(1).split(',');

    for (QString occurence : occurences) {
        QStringList coords = occurence.split('-');

        if (coords.length() < 2) {
            return;
        }

        long int start = std::stol(coords.at(0).toStdString(), nullptr, 10);
        long int end = std::stol(coords.at(1).toStdString(), nullptr, 10);

        if (start >= end || start < 0 || end > ircMessage->content().length()) {
            return;
        }

        QString name = ircMessage->content().mid(start, end - start + 1);

        vec.push_back(std::pair<long, Emote>(start, emoteManager.getTwitchEmoteById(id, name)));
    }
}

bool TwitchMessageBuilder::tryAppendEmote(QString &emoteString)
{
    Emote emote;

    if (emoteManager.bttvGlobalEmotes.tryGet(emoteString, emote)) {
        // BTTV Global Emote
        return this->appendEmote(emote);
    } else if (this->twitchChannel->bttvChannelEmotes->tryGet(emoteString, emote)) {
        // BTTV Channel Emote
        return this->appendEmote(emote);
    } else if (emoteManager.ffzGlobalEmotes.tryGet(emoteString, emote)) {
        // FFZ Global Emote
        return this->appendEmote(emote);
    } else if (this->twitchChannel->ffzChannelEmotes->tryGet(emoteString, emote)) {
        // FFZ Channel Emote
        return this->appendEmote(emote);
    } else if (emoteManager.getChatterinoEmotes().tryGet(emoteString, emote)) {
        // Chatterino Emote
        return this->appendEmote(emote);
    }

    return false;
}

bool TwitchMessageBuilder::appendEmote(Emote &emote)
{
    this->appendWord(Word(emote.url, Word::EmoteImages, emote.name, Link(Link::Url, emote.url)));

    // Perhaps check for ignored emotes here?
    return true;
}

/*
void TwitchMessageBuilder::parseTwitchBadges()
{
    const auto &channelResources = this->resources.channels[this->roomID];

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
            if (!this->resources.dynamicBadgesLoaded) {
                // Do nothing
                continue;
            }

            QString cheerAmountQS = badge.mid(5);
            std::string versionKey = cheerAmountQS.toStdString();

            try {
                auto &badgeSet = this->resources.badgeSets.at("bits");

                try {
                    auto &badgeVersion = badgeSet.versions.at(versionKey);

                    appendWord(
                        Word(badgeVersion.badgeImage1x, Word::BadgeVanity,
                             QString("Twitch " + QString::fromStdString(badgeVersion.title))));
                } catch (const std::exception &e) {
                    debug::Log("Exception caught: {} when trying to fetch badge version {}",
                               e.what(), versionKey);
                }
            } catch (const std::exception &e) {
                debug::Log("No badge set with key bits. Exception: {}", e.what());
            }
        } else if (badge == "staff/1") {
            appendWord(Word(this->resources.badgeStaff, Word::BadgeGlobalAuthority,
                            QString("Twitch Staff")));
        } else if (badge == "admin/1") {
            appendWord(Word(this->resources.badgeAdmin, Word::BadgeGlobalAuthority,
                            QString("Twitch Admin")));
        } else if (badge == "global_mod/1") {
            appendWord(Word(this->resources.badgeGlobalModerator, Word::BadgeGlobalAuthority,
                            QString("Global Moderator")));
        } else if (badge == "moderator/1") {
            // TODO: Implement custom FFZ moderator badge
            appendWord(Word(this->resources.badgeModerator, Word::BadgeChannelAuthority,
                            QString("Channel Moderator")));  // custom badge
        } else if (badge == "turbo/1") {
            appendWord(
                Word(this->resources.badgeTurbo, Word::BadgeVanity, QString("Turbo Subscriber")));
        } else if (badge == "broadcaster/1") {
            appendWord(Word(this->resources.badgeBroadcaster, Word::BadgeChannelAuthority,
                            QString("Channel Broadcaster")));
        } else if (badge == "premium/1") {
            appendWord(Word(this->resources.badgePremium, Word::BadgeVanity),
                            QString("Twitch Prime")));

        } else if (badge.startsWith("partner/")) {
            int index = badge.midRef(8).toInt();
            switch (index) {
                case 1: {
                    appendWord(
                        Word(this->resources.badgeVerified, Word::BadgeVanity, "Twitch Verified"));
                } break;
                default: {
                    printf("[TwitchMessageBuilder] Unhandled partner badge index: %d\n", index);
                } break;
            }
        } else if (badge.startsWith("subscriber/")) {
            if (channelResources.loaded == false) {
                qDebug() << "Channel resources are not loaded, can't add the subscriber badge";
                continue;
            }

            try {
                const auto &badgeSet = channelResources.badgeSets.at("subscriber");

                std::string versionKey = badge.mid(11).toStdString();

                try {
                    auto &badgeVersion = badgeSet.versions.at(versionKey);

                    appendWord(
                        Word(badgeVersion.badgeImage1x, Word::Type::BadgeSubscription,
                             QString("Twitch " + QString::fromStdString(badgeVersion.title))));
                } catch (const std::exception &e) {
                    qDebug() << "Exception caught:" << e.what()
                             << "when trying to fetch badge version " << versionKey.c_str();
                }
            } catch (const std::exception &e) {
                qDebug() << "No channel badge set with key `subscriber`. Exception: " << e.what();
            }

        } else {
            if (!this->resources.dynamicBadgesLoaded) {
                // Do nothing
                continue;
            }

            QStringList parts = badge.split('/');

            if (parts.length() != 2) {
                qDebug() << "Bad number of parts: " << parts.length() << " in " << parts;
                continue;
            }

            Word::Type badgeType = Word::Type::BadgeVanity;

            std::string badgeSetKey = parts[0].toStdString();
            std::string versionKey = parts[1].toStdString();

            try {
                auto &badgeSet = this->resources.badgeSets.at(badgeSetKey);

                try {
                    auto &badgeVersion = badgeSet.versions.at(versionKey);

                    appendWord(
                        Word(badgeVersion.badgeImage1x, badgeType, QString(),
                             QString("Twitch " + QString::fromStdString(badgeVersion.title))));
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
*/

/*
void TwitchMessageBuilder::parseChatterinoBadges()
{
    auto &badges = this->resources.chatterinoBadges;
    auto it = badges.find(this->userName.toStdString());

    if (it == badges.end()) {
        return;
    }

    const auto badge = it->second;

    this->appendWord(Word(badge->image, Word::BadgeChatterino, QString(), badge->tooltip.c_str()));
}
*/

// bool
// sortTwitchEmotes(const std::pair<long int, LazyLoadedImage *> &a,
//                 const std::pair<long int, LazyLoadedImage *> &b)
//{
//    return a.first < b.first;
//}

}  // namespace twitch
}  // namespace chatterino
