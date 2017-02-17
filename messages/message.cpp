#include "messages/message.h"
#include "colorscheme.h"
#include "emojis.h"
#include "emotes.h"
#include "fonts.h"
#include "ircmanager.h"
#include "messages/link.h"
#include "qcolor.h"
#include "resources.h"
#include "settings.h"

#include <QObjectUserData>
#include <QStringList>
#include <ctime>
#include <list>
#include <tuple>

namespace chatterino {
namespace messages {

QRegularExpression *Message::cheerRegex =
    new QRegularExpression("cheer[1-9][0-9]*");

Message::Message(const QString &text)
{
    words.push_back(Word(text, Word::Text,
                         ColorScheme::getInstance().SystemMessageColor, text,
                         QString()));
}

Message::Message(const IrcPrivateMessage &ircMessage, Channel &channel,
                 bool enablePingSound, bool isReceivedWhisper,
                 bool isSentWhisper, bool includeChannel)
{
    this->parseTime = std::chrono::system_clock::now();

    auto words = std::vector<Word>();

    auto tags = ircMessage.tags();

    auto iterator = tags.find("id");

    if (iterator != tags.end()) {
        this->id = iterator.value().toString();
    }

    // timestamps
    iterator = tags.find("tmi-sent-ts");

    std::time_t time = std::time(NULL);

    //    if (iterator != tags.end()) {
    //        time = strtoll(iterator.value().toString().toStdString().c_str(),
    //        NULL,
    //                       10);
    //    }

    char timeStampBuffer[69];

    strftime(timeStampBuffer, 69, "%H:%M", localtime(&time));
    QString timestamp = QString(timeStampBuffer);

    strftime(timeStampBuffer, 69, "%H:%M:%S", localtime(&time));
    QString timestampWithSeconds = QString(timeStampBuffer);

    words.push_back(Word(timestamp, Word::TimestampNoSeconds,
                         ColorScheme::getInstance().SystemMessageColor,
                         QString(), QString()));
    words.push_back(Word(timestampWithSeconds, Word::TimestampWithSeconds,
                         ColorScheme::getInstance().SystemMessageColor,
                         QString(), QString()));

    // mod buttons
    static QString buttonBanTooltip("Ban user");
    static QString buttonTimeoutTooltip("Timeout user");

    words.push_back(Word(Resources::getButtonBan(), Word::ButtonBan, QString(),
                         buttonBanTooltip,
                         Link(Link::UserBan, ircMessage.account())));
    words.push_back(Word(Resources::getButtonTimeout(), Word::ButtonTimeout,
                         QString(), buttonTimeoutTooltip,
                         Link(Link::UserTimeout, ircMessage.account())));

    // badges
    iterator = tags.find("badges");

    if (iterator != tags.end()) {
        auto badges = iterator.value().toString().split(',');

        for (QString badge : badges) {
            if (badge.startsWith("bits/")) {
                long long int cheer =
                    strtoll(badge.mid(5).toStdString().c_str(), NULL, 10);
                words.push_back(Word(
                    Emotes::getCheerBadge(cheer), Word::BadgeCheer, QString(),
                    QString("Twitch Cheer" + QString::number(cheer))));
            } else if (badge == "staff/1") {
                words.push_back(Word(Resources::getBadgeStaff(),
                                     Word::BadgeStaff, QString(),
                                     QString("Twitch Staff")));
            } else if (badge == "admin/1") {
                words.push_back(Word(Resources::getBadgeAdmin(),
                                     Word::BadgeAdmin, QString(),
                                     QString("Twitch Admin")));
            } else if (badge == "global_mod/1") {
                words.push_back(Word(Resources::getBadgeGlobalmod(),
                                     Word::BadgeGlobalMod, QString(),
                                     QString("Global Moderator")));
            } else if (badge == "moderator/1") {
                // TODO: implement this xD
                words.push_back(Word(
                    Resources::getBadgeTurbo(), Word::BadgeModerator, QString(),
                    QString("Channel Moderator")));  // custom badge
            } else if (badge == "turbo/1") {
                words.push_back(Word(Resources::getBadgeStaff(),
                                     Word::BadgeTurbo, QString(),
                                     QString("Turbo Subscriber")));
            } else if (badge == "broadcaster/1") {
                words.push_back(Word(Resources::getBadgeBroadcaster(),
                                     Word::BadgeBroadcaster, QString(),
                                     QString("Channel Broadcaster")));
            } else if (badge == "premium/1") {
                words.push_back(Word(Resources::getBadgePremium(),
                                     Word::BadgePremium, QString(),
                                     QString("Twitch Prime")));
            }
        }
    }

    // color
    QColor usernameColor = ColorScheme::getInstance().SystemMessageColor;

    iterator = tags.find("color");
    if (iterator != tags.end()) {
        usernameColor = QColor(iterator.value().toString());
    }

    // channel name
    if (includeChannel) {
        QString channelName("#" + channel.getName());
        words.push_back(Word(
            channelName, Word::Misc,
            ColorScheme::getInstance().SystemMessageColor, QString(channelName),
            QString(), Link(Link::Url, channel.getName() + "\n" + this->id)));
    }

    // username
    this->userName = ircMessage.nick();

    if (this->userName.isEmpty()) {
        auto iterator = tags.find("login");

        if (iterator != tags.end()) {
            this->userName = iterator.value().toString();
        }
    }

    QString displayName;

    iterator = tags.find("display-name");
    if (iterator == tags.end()) {
        displayName = ircMessage.account();
    } else {
        displayName = iterator.value().toString();
    }

    bool hasLocalizedName =
        QString::compare(displayName, ircMessage.account()) == 0;
    QString userDisplayString =
        displayName +
        (hasLocalizedName ? (" (" + ircMessage.account() + ")") : QString());

    if (isSentWhisper) {
        userDisplayString = IrcManager::account->getUsername() + " -> ";
    }

    if (isReceivedWhisper) {
        userDisplayString += " -> " + IrcManager::account->getUsername();
    }

    if (!ircMessage.isAction()) {
        userDisplayString += ": ";
    }

    words.push_back(Word(userDisplayString, Word::Username, usernameColor,
                         userDisplayString, QString()));

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
        auto emotes = iterator.value().toString().split('/');

        for (QString emote : emotes) {
            if (!emote.contains(':'))
                continue;

            QStringList parameters = emote.split(':');

            if (parameters.length() < 2)
                continue;

            long int id = std::stol(parameters.at(0).toStdString(), NULL, 10);

            QStringList occurences = parameters.at(1).split(',');

            for (QString occurence : occurences) {
                QStringList coords = occurence.split('-');

                if (coords.length() < 2)
                    continue;

                long int start =
                    std::stol(coords.at(0).toStdString(), NULL, 10);
                long int end = std::stol(coords.at(1).toStdString(), NULL, 10);

                if (start >= end || start < 0 ||
                    end > ircMessage.content().length())
                    continue;

                QString name = ircMessage.content().mid(start, end - start + 1);

                twitchEmotes.push_back(std::pair<long int, LazyLoadedImage *>(
                    start, Emotes::getTwitchEmoteById(name, id)));
            }
        }

        std::sort(twitchEmotes.begin(), twitchEmotes.end(), sortTwitchEmotes);
    }

    auto currentTwitchEmote = twitchEmotes.begin();

    // words
    QColor textColor =
        ircMessage.isAction() ? usernameColor : ColorScheme::getInstance().Text;

    QStringList splits = ircMessage.content().split(' ');

    long int i = 0;

    for (QString split : splits) {
        // twitch emote
        if (currentTwitchEmote != twitchEmotes.end() &&
            currentTwitchEmote->first == i) {
            words.push_back(Word(currentTwitchEmote->second,
                                 Word::TwitchEmoteImage,
                                 currentTwitchEmote->second->getName(),
                                 currentTwitchEmote->second->getName() +
                                     QString("\nTwitch Emote")));
            words.push_back(Word(currentTwitchEmote->second->getName(),
                                 Word::TwitchEmoteText, textColor,
                                 currentTwitchEmote->second->getName(),
                                 currentTwitchEmote->second->getName() +
                                     QString("\nTwitch Emote")));

            i += split.length() + 1;
            currentTwitchEmote = std::next(currentTwitchEmote);

            continue;
        }

        // split words
        std::vector<std::tuple<LazyLoadedImage *, QString>> parsed;

        Emojis::parseEmojis(parsed, split);
        for (const std::tuple<LazyLoadedImage *, QString> &tuple : parsed) {
            LazyLoadedImage *image = std::get<0>(tuple);

            if (image == NULL) {  // is text
                QString string = std::get<1>(tuple);

                // cheers
                if (!bits.isEmpty() && string.length() >= 6 &&
                    cheerRegex->match(string).isValid()) {
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

                    QString bitsLinkAnimated = QString(
                        "http://static-cdn.jtvnw.net/bits/dark/animated/" +
                        color + "/1");
                    QString bitsLink = QString(
                        "http://static-cdn.jtvnw.net/bits/dark/static/" +
                        color + "/1");

                    LazyLoadedImage *imageAnimated =
                        Emotes::getMiscImageFromCache().getOrAdd(
                            bitsLinkAnimated, [&bitsLinkAnimated] {
                                return new LazyLoadedImage(bitsLinkAnimated);
                            });
                    LazyLoadedImage *image =
                        Emotes::getMiscImageFromCache().getOrAdd(
                            bitsLink, [&bitsLink] {
                                return new LazyLoadedImage(bitsLink);
                            });

                    words.push_back(
                        Word(imageAnimated, Word::BitsAnimated,
                             QString("cheer"), QString("Twitch Cheer"),
                             Link(Link::Url,
                                  QString("https://blog.twitch.tv/"
                                          "introducing-cheering-celebrate-"
                                          "together-da62af41fac6"))));
                    words.push_back(
                        Word(image, Word::BitsStatic, QString("cheer"),
                             QString("Twitch Cheer"),
                             Link(Link::Url,
                                  QString("https://blog.twitch.tv/"
                                          "introducing-cheering-celebrate-"
                                          "together-da62af41fac6"))));

                    words.push_back(
                        Word(QString("x" + string.mid(5)), Word::BitsAmount,
                             bitsColor, QString(string.mid(5)),
                             QString("Twitch Cheer"),
                             Link(Link::Url,
                                  QString("https://blog.twitch.tv/"
                                          "introducing-cheering-celebrate-"
                                          "together-da62af41fac6"))));

                    continue;
                }

                // bttv / ffz emotes
                LazyLoadedImage *bttvEmote;

                // TODO: Implement this (ignored emotes)
                if (Emotes::getBttvEmotes().tryGet(string, bttvEmote) ||
                    channel.getBttvChannelEmotes().tryGet(string, bttvEmote) ||
                    Emotes::getFfzEmotes().tryGet(string, bttvEmote) ||
                    channel.getFfzChannelEmotes().tryGet(string, bttvEmote) ||
                    Emotes::getChatterinoEmotes().tryGet(string, bttvEmote)) {
                    words.push_back(Word(bttvEmote, Word::BttvEmoteImage,
                                         bttvEmote->getName(),
                                         bttvEmote->getTooltip(),
                                         Link(Link::Url, bttvEmote->getUrl())));

                    continue;
                }

                // actually just a word
                QString link = matchLink(string);

                words.push_back(
                    Word(string, Word::Text, textColor, string, QString(),
                         link.isEmpty() ? Link() : Link(Link::Url, link)));
            } else {  // is emoji
                static QString emojiTooltip("Emoji");

                words.push_back(Word(image, Word::EmojiImage, image->getName(),
                                     emojiTooltip));
                Word(image->getName(), Word::EmojiText, textColor,
                     image->getName(), emojiTooltip);
            }
        }

        i += split.length() + 1;
    }

    this->words = words;

    // TODO: Implement this xD
    //    if (!isReceivedWhisper &&
    //    AppSettings.HighlightIgnoredUsers.ContainsKey(Username))
    //    {
    //        HighlightTab = false;
    //    }
}

QString
Message::matchLink(const QString &string)
{
    // TODO: Implement this xD
    return QString();
}

bool
Message::sortTwitchEmotes(const std::pair<long int, LazyLoadedImage *> &a,
                          const std::pair<long int, LazyLoadedImage *> &b)
{
    return a.first < b.first;
}

}  // namespace messages
}  // namespace chatterino
