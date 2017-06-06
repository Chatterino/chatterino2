//#include "twitchparsemessage.h"
//#include "colorscheme.h"
//#include "emojis.h"
//#include "emotemanager.h"
//#include "ircmanager.h"
//#include "resources.h"
//#include "twitch/twitchmessagebuilder.h"
//
//#include <QRegularExpression>
//
// using namespace chatterino::messages;
//
// namespace chatterino {
// namespace twitch {
// SharedMessage
// twitchParseMessage(const Communi::IrcPrivateMessage *ircMessage,
//                   Channel *channel, const MessageParseArgs &args)
//{
//    TwitchMessageBuilder b;
//
//    // timestamp
//    b.appendTimestamp();
//
//    auto tags = ircMessage->tags();
//
//    auto iterator = tags.find("id");
//
//    if (iterator != tags.end()) {
//        b.messageId = iterator.value().toString();
//    }
//
//    // timestamps
//    iterator = tags.find("tmi-sent-ts");
//
//    // mod buttons
//    static QString buttonBanTooltip("Ban user");
//    static QString buttonTimeoutTooltip("Timeout user");
//
//    b.appendWord(Word(Resources::getButtonBan(), Word::ButtonBan, QString(),
//                      buttonBanTooltip,
//                      Link(Link::UserBan, ircMessage->account())));
//    b.appendWord(Word(Resources::getButtonTimeout(), Word::ButtonTimeout,
//                      QString(), buttonTimeoutTooltip,
//                      Link(Link::UserTimeout, ircMessage->account())));
//
//    // badges
//    iterator = tags.find("badges");
//
//    if (iterator != tags.end()) {
//        auto badges = iterator.value().toString().split(',');
//
//        b.appendTwitchBadges(badges);
//    }
//
//    // color
//    QColor usernameColor = ColorScheme::getInstance().SystemMessageColor;
//
//    iterator = tags.find("color");
//    if (iterator != tags.end()) {
//        usernameColor = QColor(iterator.value().toString());
//    }
//
//    // channel name
//    if (args.includeChannelName) {
//        QString channelName("#" + channel->getName());
//        b.appendWord(
//            Word(channelName, Word::Misc,
//                 ColorScheme::getInstance().SystemMessageColor,
//                 QString(channelName), QString(),
//                 Link(Link::Url, channel->getName() + "\n" + b.messageId)));
//    }
//
//    // username
//    b.userName = ircMessage->nick();
//
//    if (b.userName.isEmpty()) {
//        b.userName = tags.value(QLatin1String("login")).toString();
//    }
//
//    QString displayName;
//
//    iterator = tags.find("display-name");
//    if (iterator == tags.end()) {
//        displayName = ircMessage->account();
//    } else {
//        displayName = iterator.value().toString();
//    }
//
//    bool hasLocalizedName =
//        QString::compare(displayName, ircMessage->account()) == 0;
//    QString userDisplayString =
//        displayName +
//        (hasLocalizedName ? (" (" + ircMessage->account() + ")") : QString());
//
//    if (args.isSentWhisper) {
//        userDisplayString +=
//            IrcManager::getInstance().getUser().getUserName() + " -> ";
//    }
//
//    if (args.isReceivedWhisper) {
//        userDisplayString +=
//            " -> " + IrcManager::getInstance().getUser().getUserName();
//    }
//
//    if (!ircMessage->isAction()) {
//        userDisplayString += ": ";
//    }
//
//    b.appendWord(Word(userDisplayString, Word::Username, usernameColor,
//                      userDisplayString, QString()));
//
//    // highlights
//    // TODO: implement this xD
//
//    // bits
//    QString bits = "";
//
//    iterator = tags.find("bits");
//    if (iterator != tags.end()) {
//        bits = iterator.value().toString();
//    }
//
//    // twitch emotes
//    std::vector<std::pair<long int, LazyLoadedImage *>> twitchEmotes;
//
//    iterator = tags.find("emotes");
//
//    if (iterator != tags.end()) {
//        auto emotes = iterator.value().toString().split('/');
//
//        for (QString emote : emotes) {
//            if (!emote.contains(':'))
//                continue;
//
//            QStringList parameters = emote.split(':');
//
//            if (parameters.length() < 2)
//                continue;
//
//            long int id = std::stol(parameters.at(0).toStdString(), NULL, 10);
//
//            QStringList occurences = parameters.at(1).split(',');
//
//            for (QString occurence : occurences) {
//                QStringList coords = occurence.split('-');
//
//                if (coords.length() < 2)
//                    continue;
//
//                long int start =
//                    std::stol(coords.at(0).toStdString(), NULL, 10);
//                long int end = std::stol(coords.at(1).toStdString(), NULL,
//                10);
//
//                if (start >= end || start < 0 ||
//                    end > ircMessage->content().length())
//                    continue;
//
//                QString name =
//                    ircMessage->content().mid(start, end - start + 1);
//
//                twitchEmotes.push_back(std::pair<long int, LazyLoadedImage *>(
//                    start,
//                    EmoteManager::getInstance().getTwitchEmoteById(name,
//                    id)));
//            }
//        }
//
//        std::sort(twitchEmotes.begin(), twitchEmotes.end(), sortTwitchEmotes);
//    }
//
//    auto currentTwitchEmote = twitchEmotes.begin();
//
//    // words
//    QColor textColor = ircMessage->isAction() ? usernameColor
//                                              :
//                                              ColorScheme::getInstance().Text;
//
//    QStringList splits = ircMessage->content().split(' ');
//
//    long int i = 0;
//
//    for (QString split : splits) {
//        // twitch emote
//        if (currentTwitchEmote != twitchEmotes.end() &&
//            currentTwitchEmote->first == i) {
//            b.appendWord(Word(currentTwitchEmote->second,
//                              Word::TwitchEmoteImage,
//                              currentTwitchEmote->second->getName(),
//                              currentTwitchEmote->second->getName() +
//                                  QString("\nTwitch Emote")));
//            b.appendWord(Word(currentTwitchEmote->second->getName(),
//                              Word::TwitchEmoteText, textColor,
//                              currentTwitchEmote->second->getName(),
//                              currentTwitchEmote->second->getName() +
//                                  QString("\nTwitch Emote")));
//
//            i += split.length() + 1;
//            currentTwitchEmote = std::next(currentTwitchEmote);
//
//            continue;
//        }
//
//        // split words
//        std::vector<std::tuple<LazyLoadedImage *, QString>> parsed;
//
//        Emojis::parseEmojis(parsed, split);
//
//        for (const std::tuple<LazyLoadedImage *, QString> &tuple : parsed) {
//            LazyLoadedImage *image = std::get<0>(tuple);
//
//            if (image == NULL) {  // is text
//                QString string = std::get<1>(tuple);
//
//                static QRegularExpression cheerRegex("cheer[1-9][0-9]*");
//
//                // cheers
//                if (!bits.isEmpty() && string.length() >= 6 &&
//                    cheerRegex.match(string).isValid()) {
//                    auto cheer = string.mid(5).toInt();
//
//                    QString color;
//
//                    QColor bitsColor;
//
//                    if (cheer >= 10000) {
//                        color = "red";
//                        bitsColor = QColor::fromHslF(0, 1, 0.5);
//                    } else if (cheer >= 5000) {
//                        color = "blue";
//                        bitsColor = QColor::fromHslF(0.61, 1, 0.4);
//                    } else if (cheer >= 1000) {
//                        color = "green";
//                        bitsColor = QColor::fromHslF(0.5, 1, 0.5);
//                    } else if (cheer >= 100) {
//                        color = "purple";
//                        bitsColor = QColor::fromHslF(0.8, 1, 0.5);
//                    } else {
//                        color = "gray";
//                        bitsColor = QColor::fromHslF(0.5f, 0.5f, 0.5f);
//                    }
//
//                    QString bitsLinkAnimated = QString(
//                        "http://static-cdn.jtvnw.net/bits/dark/animated/" +
//                        color + "/1");
//                    QString bitsLink = QString(
//                        "http://static-cdn.jtvnw.net/bits/dark/static/" +
//                        color + "/1");
//
//                    LazyLoadedImage *imageAnimated =
//                        EmoteManager::getInstance()
//                            .getMiscImageFromCache()
//                            .getOrAdd(bitsLinkAnimated, [&bitsLinkAnimated] {
//                                return new LazyLoadedImage(bitsLinkAnimated);
//                            });
//                    LazyLoadedImage *image =
//                        EmoteManager::getInstance()
//                            .getMiscImageFromCache()
//                            .getOrAdd(bitsLink, [&bitsLink] {
//                                return new LazyLoadedImage(bitsLink);
//                            });
//
//                    b.appendWord(
//                        Word(imageAnimated, Word::BitsAnimated,
//                             QString("cheer"), QString("Twitch Cheer"),
//                             Link(Link::Url,
//                                  QString("https://blog.twitch.tv/"
//                                          "introducing-cheering-celebrate-"
//                                          "together-da62af41fac6"))));
//                    b.appendWord(
//                        Word(image, Word::BitsStatic, QString("cheer"),
//                             QString("Twitch Cheer"),
//                             Link(Link::Url,
//                                  QString("https://blog.twitch.tv/"
//                                          "introducing-cheering-celebrate-"
//                                          "together-da62af41fac6"))));
//
//                    b.appendWord(
//                        Word(QString("x" + string.mid(5)), Word::BitsAmount,
//                             bitsColor, QString(string.mid(5)),
//                             QString("Twitch Cheer"),
//                             Link(Link::Url,
//                                  QString("https://blog.twitch.tv/"
//                                          "introducing-cheering-celebrate-"
//                                          "together-da62af41fac6"))));
//
//                    continue;
//                }
//
//                // bttv / ffz emotes
//                LazyLoadedImage *bttvEmote;
//
//                // TODO: Implement this (ignored emotes)
//                if (EmoteManager::getInstance().getBttvEmotes().tryGet(
//                        string, bttvEmote) ||
//                    channel->getBttvChannelEmotes().tryGet(string, bttvEmote)
//                    ||
//                    EmoteManager::getInstance().getFfzEmotes().tryGet(
//                        string, bttvEmote) ||
//                    channel->getFfzChannelEmotes().tryGet(string, bttvEmote)
//                    ||
//                    EmoteManager::getInstance().getChatterinoEmotes().tryGet(
//                        string, bttvEmote)) {
//                    b.appendWord(Word(bttvEmote, Word::BttvEmoteImage,
//                                      bttvEmote->getName(),
//                                      bttvEmote->getTooltip(),
//                                      Link(Link::Url, bttvEmote->getUrl())));
//
//                    continue;
//                }
//
//                // actually just a word
//                QString link = b.matchLink(string);
//
//                b.appendWord(
//                    Word(string, Word::Text, textColor, string, QString(),
//                         link.isEmpty() ? Link() : Link(Link::Url, link)));
//            } else {  // is emoji
//                static QString emojiTooltip("Emoji");
//
//                b.appendWord(Word(image, Word::EmojiImage, image->getName(),
//                                  emojiTooltip));
//                Word(image->getName(), Word::EmojiText, textColor,
//                     image->getName(), emojiTooltip);
//            }
//        }
//
//        i += split.length() + 1;
//    }
//
//    // TODO: Implement this xD
//    //    if (!isReceivedWhisper &&
//    //    AppSettings.HighlightIgnoredUsers.ContainsKey(Username))
//    //    {
//    //        HighlightTab = false;
//    //    }
//
//    return b.build();
//}
//
// bool
// sortTwitchEmotes(const std::pair<long int, LazyLoadedImage *> &a,
//                 const std::pair<long int, LazyLoadedImage *> &b)
//{
//    return a.first < b.first;
//}
//}
//}
//
