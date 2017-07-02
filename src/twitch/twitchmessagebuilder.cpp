#include "twitch/twitchmessagebuilder.hpp"
#include "colorscheme.hpp"
#include "emotemanager.hpp"
#include "ircmanager.hpp"
#include "resources.hpp"
#include "windowmanager.hpp"

using namespace chatterino::messages;

namespace chatterino {
namespace twitch {

TwitchMessageBuilder::TwitchMessageBuilder(Channel *_channel, Resources &_resources,
                                           EmoteManager &_emoteManager,
                                           WindowManager &_windowManager,
                                           const Communi::IrcPrivateMessage *_ircMessage,
                                           const messages::MessageParseArgs &_args)
    : channel(_channel)
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
    // The timestamp is always appended to the builder
    // Whether or not will be rendered is decided/checked later
    this->appendTimestamp();

    this->parseMessageID();

    this->parseRoomID();

    this->appendModerationButtons();

    this->parseTwitchBadges();

    if (this->args.includeChannelName) {
        this->parseChannelName();
    }

    this->parseUsername();

    // highlights
    // TODO: implement this xD

    // bits
    QString bits = "";

    auto iterator = this->tags.find("bits");
    if (iterator != this->tags.end()) {
        bits = iterator.value().toString();
    }

    // twitch emotes
    std::vector<std::pair<long int, LazyLoadedImage *>> twitchEmotes;

    iterator = this->tags.find("emotes");
    if (iterator != this->tags.end()) {
        QStringList emoteString = iterator.value().toString().split('/');

        for (QString emote : emoteString) {
            this->appendTwitchEmote(ircMessage, emote, twitchEmotes, emoteManager);
        }

        struct {
            bool operator()(const std::pair<long int, messages::LazyLoadedImage *> &lhs,
                            const std::pair<long int, messages::LazyLoadedImage *> &rhs)
            {
                return lhs.first < rhs.first;
            }
        } customLess;

        std::sort(twitchEmotes.begin(), twitchEmotes.end(), customLess);
    }

    auto currentTwitchEmote = twitchEmotes.begin();

    // words
    QColor textColor = ircMessage->isAction() ? this->usernameColor : this->colorScheme.Text;

    const QString &originalMessage = ircMessage->content();
    this->originalMessage = originalMessage;
    QStringList splits = originalMessage.split(' ');

    long int i = 0;

    for (QString split : splits) {
        // twitch emote
        if (currentTwitchEmote != twitchEmotes.end() && currentTwitchEmote->first == i) {
            this->appendWord(
                Word(currentTwitchEmote->second, Word::TwitchEmoteImage,
                     currentTwitchEmote->second->getName(),
                     currentTwitchEmote->second->getName() + QString("\nTwitch Emote")));
            this->appendWord(
                Word(currentTwitchEmote->second->getName(), Word::TwitchEmoteText, textColor,
                     currentTwitchEmote->second->getName(),
                     currentTwitchEmote->second->getName() + QString("\nTwitch Emote")));

            i += split.length() + 1;
            currentTwitchEmote = std::next(currentTwitchEmote);

            continue;
        }

        // split words
        std::vector<std::tuple<LazyLoadedImage *, QString>> parsed;

        // Parse emojis and take all non-emojis and put them in parsed as full text-words
        emoteManager.parseEmojis(parsed, split);

        for (const std::tuple<LazyLoadedImage *, QString> &tuple : parsed) {
            LazyLoadedImage *image = std::get<0>(tuple);

            if (image == nullptr) {  // is text
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

                    LazyLoadedImage *imageAnimated = emoteManager.getMiscImageFromCache().getOrAdd(
                        bitsLinkAnimated, [this, &bitsLinkAnimated] {
                            return new LazyLoadedImage(this->emoteManager, this->windowManager,
                                                       bitsLinkAnimated);
                        });
                    LazyLoadedImage *image =
                        emoteManager.getMiscImageFromCache().getOrAdd(bitsLink, [this, &bitsLink] {
                            return new LazyLoadedImage(this->emoteManager, this->windowManager,
                                                       bitsLink);
                        });

                    this->appendWord(Word(imageAnimated, Word::BitsAnimated, QString("cheer"),
                                          QString("Twitch Cheer"),
                                          Link(Link::Url, QString("https://blog.twitch.tv/"
                                                                  "introducing-cheering-celebrate-"
                                                                  "together-da62af41fac6"))));
                    this->appendWord(Word(image, Word::BitsStatic, QString("cheer"),
                                          QString("Twitch Cheer"),
                                          Link(Link::Url, QString("https://blog.twitch.tv/"
                                                                  "introducing-cheering-celebrate-"
                                                                  "together-da62af41fac6"))));

                    this->appendWord(Word(QString("x" + string.mid(5)), Word::BitsAmount, bitsColor,
                                          QString(string.mid(5)), QString("Twitch Cheer"),
                                          Link(Link::Url, QString("https://blog.twitch.tv/"
                                                                  "introducing-cheering-celebrate-"
                                                                  "together-da62af41fac6"))));

                    continue;
                }

                // bttv / ffz emotes
                LazyLoadedImage *bttvEmote;

                // TODO: Implement ignored emotes
                // Format of ignored emotes:
                // Emote name: "forsenPuke" - if string in ignoredEmotes
                // Will match emote regardless of source (i.e. bttv, ffz)
                // Emote source + name: "bttv:nyanPls"
                if (emoteManager.getBTTVEmotes().tryGet(string, bttvEmote) ||
                    this->channel->getBttvChannelEmotes().tryGet(string, bttvEmote) ||
                    emoteManager.getFFZEmotes().tryGet(string, bttvEmote) ||
                    this->channel->getFfzChannelEmotes().tryGet(string, bttvEmote) ||
                    emoteManager.getChatterinoEmotes().tryGet(string, bttvEmote)) {
                    this->appendWord(Word(bttvEmote, Word::BttvEmoteImage, bttvEmote->getName(),
                                          bttvEmote->getTooltip(),
                                          Link(Link::Url, bttvEmote->getUrl())));

                    continue;
                }

                // actually just a word
                QString link = this->matchLink(string);

                this->appendWord(Word(string, Word::Text, textColor, string, QString(),
                                      link.isEmpty() ? Link() : Link(Link::Url, link)));
            } else {  // is emoji
                static QString emojiTooltip("Emoji");

                this->appendWord(Word(image, Word::EmojiImage, image->getName(), emojiTooltip));
                Word(image->getName(), Word::EmojiText, textColor, image->getName(), emojiTooltip);
            }
        }

        i += split.length() + 1;
    }

    // TODO: Implement this xD
    //    if (!isReceivedWhisper &&
    //    AppSettings.HighlightIgnoredUsers.ContainsKey(Username))
    //    {
    //        HighlightTab = false;
    //    }

    return this->build();
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

        if (this->channel->roomID.empty()) {
            this->channel->roomID = this->roomID;
        }
    }
}

void TwitchMessageBuilder::parseChannelName()
{
    QString channelName("#" + this->channel->getName());
    this->appendWord(Word(channelName, Word::Misc, this->colorScheme.SystemMessageColor,
                          QString(channelName), QString(),
                          Link(Link::Url, this->channel->getName() + "\n" + this->messageID)));
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

    QString displayName;

    iterator = this->tags.find("display-name");
    if (iterator == this->tags.end()) {
        displayName = this->userName;
    } else {
        displayName = iterator.value().toString();
    }

    bool hasLocalizedName = QString::compare(displayName, ircMessage->account()) == 0;
    QString userDisplayString =
        displayName + (hasLocalizedName ? (" (" + ircMessage->account() + ")") : QString());

    if (this->args.isSentWhisper) {
        // TODO(pajlada): Re-implement
        // userDisplayString += IrcManager::getInstance().getUser().getUserName();
    }

    if (this->args.isReceivedWhisper) {
        // TODO(pajlada): Re-implement
        // userDisplayString += " -> " + IrcManager::getInstance().getUser().getUserName();
    }

    if (!ircMessage->isAction()) {
        userDisplayString += ": ";
    }

    this->appendWord(Word(userDisplayString, Word::Username, this->usernameColor, userDisplayString,
                          QString(), Link(Link::UserInfo, this->userName)));
}

void TwitchMessageBuilder::appendModerationButtons()
{
    // mod buttons
    static QString buttonBanTooltip("Ban user");
    static QString buttonTimeoutTooltip("Timeout user");

    this->appendWord(Word(this->resources.buttonBan, Word::ButtonBan, QString(), buttonBanTooltip,
                          Link(Link::UserBan, ircMessage->account())));
    this->appendWord(Word(this->resources.buttonTimeout, Word::ButtonTimeout, QString(),
                          buttonTimeoutTooltip, Link(Link::UserTimeout, ircMessage->account())));
}

void TwitchMessageBuilder::appendTwitchEmote(
    const Communi::IrcPrivateMessage *ircMessage, const QString &emote,
    std::vector<std::pair<long int, messages::LazyLoadedImage *>> &vec, EmoteManager &emoteManager)
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

        vec.push_back(std::pair<long int, LazyLoadedImage *>(
            start, emoteManager.getTwitchEmoteById(name, id)));
    }
}

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
                        Word(badgeVersion.badgeImage1x, Word::BadgeVanity, QString(),
                             QString("Twitch " + QString::fromStdString(badgeVersion.title))));
                } catch (const std::exception &e) {
                    qDebug() << "Exception caught:" << e.what()
                             << "when trying to fetch badge version " << versionKey.c_str();
                }
            } catch (const std::exception &e) {
                qDebug() << "No badge set with key"
                         << "bits"
                         << ". Exception: " << e.what();
            }
        } else if (badge == "staff/1") {
            appendWord(Word(this->resources.badgeStaff, Word::BadgeGlobalAuthority, QString(),
                            QString("Twitch Staff")));
        } else if (badge == "admin/1") {
            appendWord(Word(this->resources.badgeAdmin, Word::BadgeGlobalAuthority, QString(),
                            QString("Twitch Admin")));
        } else if (badge == "global_mod/1") {
            appendWord(Word(this->resources.badgeGlobalModerator, Word::BadgeGlobalAuthority,
                            QString(), QString("Global Moderator")));
        } else if (badge == "moderator/1") {
            // TODO: Implement custom FFZ moderator badge
            appendWord(Word(this->resources.badgeModerator, Word::BadgeChannelAuthority, QString(),
                            QString("Channel Moderator")));  // custom badge
        } else if (badge == "turbo/1") {
            appendWord(Word(this->resources.badgeTurbo, Word::BadgeVanity, QString(),
                            QString("Turbo Subscriber")));
        } else if (badge == "broadcaster/1") {
            appendWord(Word(this->resources.badgeBroadcaster, Word::BadgeChannelAuthority,
                            QString(), QString("Channel Broadcaster")));
        } else if (badge == "premium/1") {
            appendWord(Word(this->resources.badgePremium, Word::BadgeVanity, QString(),
                            QString("Twitch Prime")));

        } else if (badge.startsWith("partner/")) {
            int index = badge.midRef(8).toInt();
            switch (index) {
                case 1: {
                    appendWord(Word(this->resources.badgeVerified, Word::BadgeVanity, QString(),
                                    "Twitch Verified"));
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
                        Word(badgeVersion.badgeImage1x, Word::Type::BadgeSubscription, QString(),
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

// bool
// sortTwitchEmotes(const std::pair<long int, LazyLoadedImage *> &a,
//                 const std::pair<long int, LazyLoadedImage *> &b)
//{
//    return a.first < b.first;
//}

}  // namespace twitch
}  // namespace chatterino
