#include "twitch/twitchmessagebuilder.hpp"
#include "colorscheme.hpp"
#include "emojis.hpp"
#include "emotemanager.hpp"
#include "ircmanager.hpp"
#include "resources.hpp"
#include "windowmanager.hpp"

using namespace chatterino::messages;

namespace chatterino {
namespace twitch {

TwitchMessageBuilder::TwitchMessageBuilder()
    : MessageBuilder()
    , messageId()
    , userName()
{
}

// When a twitch message is being parsed it makes sense for the builder to have access to:
//  - The IRC Message
//  - Message-specific parsing arguments (i.e. if ping sounds are disabled)
//  - The Channel that the message originated from
//  - The resources class for Badges, Moderation buttons
//  - The Emote Manager for all different kinds of emotes
SharedMessage TwitchMessageBuilder::parse(const Communi::IrcPrivateMessage *ircMessage,  //
                                          Channel *channel,                              //
                                          const MessageParseArgs &args,                  //
                                          Resources &resources,                          //
                                          EmoteManager &emoteManager,                    //
                                          WindowManager &windowManager)
{
    TwitchMessageBuilder b;

    // The timestamp is always appended to the builder
    // Whether or not will be rendered is decided/checked later
    b.appendTimestamp();

    auto tags = ircMessage->tags();

    auto iterator = tags.find("id");

    if (iterator != tags.end()) {
        b.messageId = iterator.value().toString();
    }

    // room id
    iterator = tags.find("room-id");
    std::string roomID;
    if (iterator != std::end(tags)) {
        roomID = iterator.value().toString().toStdString();
    }

    // timestamps
    iterator = tags.find("tmi-sent-ts");

    b.appendModerationWords(ircMessage, resources);

    // badges
    iterator = tags.find("badges");

    const auto &channelResources = resources.channels[roomID];

    if (iterator != tags.end()) {
        QStringList badges = iterator.value().toString().split(',');

        b.appendTwitchBadges(badges, resources, emoteManager, channelResources);
    }

    // color
    QColor usernameColor = ColorScheme::getInstance().SystemMessageColor;

    iterator = tags.find("color");
    if (iterator != tags.end()) {
        usernameColor = QColor(iterator.value().toString());
    }

    // channel name
    if (args.includeChannelName) {
        QString channelName("#" + channel->getName());
        b.appendWord(Word(channelName, Word::Misc, ColorScheme::getInstance().SystemMessageColor,
                          QString(channelName), QString(),
                          Link(Link::Url, channel->getName() + "\n" + b.messageId)));
    }

    // username
    b.userName = ircMessage->nick();

    if (b.userName.isEmpty()) {
        b.userName = tags.value(QLatin1String("login")).toString();
    }

    QString displayName;

    iterator = tags.find("display-name");
    if (iterator == tags.end()) {
        displayName = b.userName;
    } else {
        displayName = iterator.value().toString();
    }

    bool hasLocalizedName = QString::compare(displayName, ircMessage->account()) == 0;
    QString userDisplayString =
        displayName + (hasLocalizedName ? (" (" + ircMessage->account() + ")") : QString());

    if (args.isSentWhisper) {
        // TODO(pajlada): Re-implement
        // userDisplayString += IrcManager::getInstance().getUser().getUserName();
    }

    if (args.isReceivedWhisper) {
        // TODO(pajlada): Re-implement
        // userDisplayString += " -> " + IrcManager::getInstance().getUser().getUserName();
    }

    if (!ircMessage->isAction()) {
        userDisplayString += ": ";
    }

    b.appendWord(Word(userDisplayString, Word::Username, usernameColor, userDisplayString,
                      QString(), Link(Link::UserInfo, b.userName)));

    // highlights
    // TODO: implement this xD

    // bits
    QString bits = "";

    iterator = tags.find("bits");
    if (iterator != tags.end()) {
        bits = iterator.value().toString();
    }

    // twitch emotes
    std::vector<std::pair<long int, LazyLoadedImage *>> twitchEmotes;

    iterator = tags.find("emotes");
    if (iterator != tags.end()) {
        QStringList emoteString = iterator.value().toString().split('/');

        for (QString emote : emoteString) {
            b.appendTwitchEmote(ircMessage, emote, twitchEmotes, emoteManager);
        }

        struct {
            bool operator()(const std::pair<long int, messages::LazyLoadedImage *> &a,
                            const std::pair<long int, messages::LazyLoadedImage *> &b)
            {
                return a.first < b.first;
            }
        } customLess;

        std::sort(twitchEmotes.begin(), twitchEmotes.end(), customLess);
    }

    auto currentTwitchEmote = twitchEmotes.begin();

    // words
    QColor textColor = ircMessage->isAction() ? usernameColor : ColorScheme::getInstance().Text;

    const QString &originalMessage = ircMessage->content();
    b.originalMessage = originalMessage;
    QStringList splits = originalMessage.split(' ');

    long int i = 0;

    for (QString split : splits) {
        // twitch emote
        if (currentTwitchEmote != twitchEmotes.end() && currentTwitchEmote->first == i) {
            b.appendWord(Word(currentTwitchEmote->second, Word::TwitchEmoteImage,
                              currentTwitchEmote->second->getName(),
                              currentTwitchEmote->second->getName() + QString("\nTwitch Emote")));
            b.appendWord(Word(currentTwitchEmote->second->getName(), Word::TwitchEmoteText,
                              textColor, currentTwitchEmote->second->getName(),
                              currentTwitchEmote->second->getName() + QString("\nTwitch Emote")));

            i += split.length() + 1;
            currentTwitchEmote = std::next(currentTwitchEmote);

            continue;
        }

        // split words
        std::vector<std::tuple<LazyLoadedImage *, QString>> parsed;

        Emojis::parseEmojis(parsed, split);

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
                        bitsLinkAnimated, [&emoteManager, &windowManager, &bitsLinkAnimated] {
                            return new LazyLoadedImage(emoteManager, windowManager,
                                                       bitsLinkAnimated);
                        });
                    LazyLoadedImage *image = emoteManager.getMiscImageFromCache().getOrAdd(
                        bitsLink, [&emoteManager, &windowManager, &bitsLink] {
                            return new LazyLoadedImage(emoteManager, windowManager, bitsLink);
                        });

                    b.appendWord(Word(imageAnimated, Word::BitsAnimated, QString("cheer"),
                                      QString("Twitch Cheer"),
                                      Link(Link::Url, QString("https://blog.twitch.tv/"
                                                              "introducing-cheering-celebrate-"
                                                              "together-da62af41fac6"))));
                    b.appendWord(Word(image, Word::BitsStatic, QString("cheer"),
                                      QString("Twitch Cheer"),
                                      Link(Link::Url, QString("https://blog.twitch.tv/"
                                                              "introducing-cheering-celebrate-"
                                                              "together-da62af41fac6"))));

                    b.appendWord(Word(QString("x" + string.mid(5)), Word::BitsAmount, bitsColor,
                                      QString(string.mid(5)), QString("Twitch Cheer"),
                                      Link(Link::Url, QString("https://blog.twitch.tv/"
                                                              "introducing-cheering-celebrate-"
                                                              "together-da62af41fac6"))));

                    continue;
                }

                // bttv / ffz emotes
                LazyLoadedImage *bttvEmote;

                // TODO: Implement this (ignored emotes)
                if (emoteManager.getBTTVEmotes().tryGet(string, bttvEmote) ||
                    channel->getBttvChannelEmotes().tryGet(string, bttvEmote) ||
                    emoteManager.getFFZEmotes().tryGet(string, bttvEmote) ||
                    channel->getFfzChannelEmotes().tryGet(string, bttvEmote) ||
                    emoteManager.getChatterinoEmotes().tryGet(string, bttvEmote)) {
                    b.appendWord(Word(bttvEmote, Word::BttvEmoteImage, bttvEmote->getName(),
                                      bttvEmote->getTooltip(),
                                      Link(Link::Url, bttvEmote->getUrl())));

                    continue;
                }

                // actually just a word
                QString link = b.matchLink(string);

                b.appendWord(Word(string, Word::Text, textColor, string, QString(),
                                  link.isEmpty() ? Link() : Link(Link::Url, link)));
            } else {  // is emoji
                static QString emojiTooltip("Emoji");

                b.appendWord(Word(image, Word::EmojiImage, image->getName(), emojiTooltip));
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

    return b.build();
}

void TwitchMessageBuilder::appendModerationWords(const Communi::IrcPrivateMessage *ircMessage,
                                                 const Resources &resources)
{
    // mod buttons
    static QString buttonBanTooltip("Ban user");
    static QString buttonTimeoutTooltip("Timeout user");

    this->appendWord(Word(resources.buttonBan, Word::ButtonBan, QString(), buttonBanTooltip,
                          Link(Link::UserBan, ircMessage->account())));
    this->appendWord(Word(resources.buttonTimeout, Word::ButtonTimeout, QString(),
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

void TwitchMessageBuilder::appendTwitchBadges(const QStringList &badges, Resources &resources,
                                              EmoteManager &emoteManager,
                                              const Resources::Channel &channelResources)
{
    for (QString badge : badges) {
        if (badge.isEmpty()) {
            continue;
        }

        if (badge.startsWith("bits/")) {
            if (!resources.dynamicBadgesLoaded) {
                // Do nothing
                continue;
            }

            QString cheerAmountQS = badge.mid(5);
            std::string versionKey = cheerAmountQS.toStdString();

            try {
                auto &badgeSet = resources.badgeSets.at("bits");

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
            appendWord(Word(resources.badgeStaff, Word::BadgeGlobalAuthority, QString(),
                            QString("Twitch Staff")));
        } else if (badge == "admin/1") {
            appendWord(Word(resources.badgeAdmin, Word::BadgeGlobalAuthority, QString(),
                            QString("Twitch Admin")));
        } else if (badge == "global_mod/1") {
            appendWord(Word(resources.badgeGlobalModerator, Word::BadgeGlobalAuthority, QString(),
                            QString("Global Moderator")));
        } else if (badge == "moderator/1") {
            // TODO: Implement custom FFZ moderator badge
            appendWord(Word(resources.badgeModerator, Word::BadgeChannelAuthority, QString(),
                            QString("Channel Moderator")));  // custom badge
        } else if (badge == "turbo/1") {
            appendWord(Word(resources.badgeTurbo, Word::BadgeVanity, QString(),
                            QString("Turbo Subscriber")));
        } else if (badge == "broadcaster/1") {
            appendWord(Word(resources.badgeBroadcaster, Word::BadgeChannelAuthority, QString(),
                            QString("Channel Broadcaster")));
        } else if (badge == "premium/1") {
            appendWord(Word(resources.badgePremium, Word::BadgeVanity, QString(),
                            QString("Twitch Prime")));

        } else if (badge.startsWith("partner/")) {
            int index = badge.midRef(8).toInt();
            switch (index) {
                case 1: {
                    appendWord(Word(resources.badgeVerified, Word::BadgeVanity, QString(),
                                    "Twitch Verified"));
                } break;
                default: {
                    printf("[TwitchMessageBuilder] Unhandled partner badge index: %d\n", index);
                } break;
            }
        } else if (badge.startsWith("subscriber/")) {
            qDebug() << "Subscriber badge:" << badge;
            int index = badge.midRef(11).toInt();
            // TODO: Implement subscriber badges here
            switch (index) {
                default: {
                    // printf("[TwitchMessageBuilder] Unhandled subscriber badge index: %d\n",
                    // index);
                } break;
            }
        } else {
            if (!resources.dynamicBadgesLoaded) {
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
                auto &badgeSet = resources.badgeSets.at(badgeSetKey);

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
