#include "message.h"
#include "appsettings.h"
#include "colorscheme.h"
#include "emojis.h"
#include "emotes.h"
#include "fonts.h"
#include "ircmanager.h"
#include "link.h"
#include "qcolor.h"
#include "resources.h"

#include <QStringList>
#include <ctime>
#include <list>
#include <tuple>

#define MARGIN_LEFT 8
#define MARGIN_RIGHT 8
#define MARGIN_TOP 8
#define MARGIN_BOTTOM 8

QRegularExpression *Message::cheerRegex =
    new QRegularExpression("cheer[1-9][0-9]*");

Message::Message(const QString &text)
    : m_wordParts(new std::list<WordPart>())
{
}

Message::Message(const IrcPrivateMessage &ircMessage, const Channel &channel,
                 bool enablePingSound, bool isReceivedWhisper,
                 bool isSentWhisper, bool includeChannel)
    : m_wordParts(new std::list<WordPart>())
{
    m_parseTime = std::chrono::system_clock::now();

    auto words = std::vector<Word>();

    auto tags = ircMessage.tags();

    auto iterator = tags.find("id");

    if (iterator != tags.end()) {
        m_id = iterator.value().toString();
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
                         ColorScheme::instance().SystemMessageColor, QString(),
                         QString()));
    words.push_back(Word(timestampWithSeconds, Word::TimestampWithSeconds,
                         ColorScheme::instance().SystemMessageColor, QString(),
                         QString()));

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
                words.push_back(Word(Resources::badgeStaff(), Word::BadgeStaff,
                                     QString(), QString("Twitch Staff")));
            } else if (badge == "admin/1") {
                words.push_back(Word(Resources::badgeAdmin(), Word::BadgeAdmin,
                                     QString(), QString("Twitch Admin")));
            } else if (badge == "global_mod/1") {
                words.push_back(Word(Resources::badgeGlobalmod(),
                                     Word::BadgeGlobalMod, QString(),
                                     QString("Global Moderator")));
            } else if (badge == "moderator/1") {
#pragma message WARN("xD")
                words.push_back(Word(
                    Resources::badgeTurbo(), Word::BadgeModerator, QString(),
                    QString("Channel Moderator")));  // custom badge
            } else if (badge == "turbo/1") {
                words.push_back(Word(Resources::badgeStaff(), Word::BadgeTurbo,
                                     QString(), QString("Turbo Subscriber")));
            } else if (badge == "broadcaster/1") {
                words.push_back(Word(Resources::badgeBroadcaster(),
                                     Word::BadgeBroadcaster, QString(),
                                     QString("Channel Broadcaster")));
            } else if (badge == "premium/1") {
                words.push_back(Word(Resources::badgePremium(),
                                     Word::BadgePremium, QString(),
                                     QString("Twitch Prime")));
            }
        }
    }

    // color
    QColor usernameColor = ColorScheme::instance().SystemMessageColor;

    iterator = tags.find("color");
    if (iterator != tags.end()) {
        usernameColor = QColor(iterator.value().toString());
    }

    // channel name
    if (includeChannel) {
        QString channelName("#" + channel.name());
        words.push_back(Word(channelName, Word::Misc,
                             ColorScheme::instance().SystemMessageColor,
                             QString(channelName), QString(),
                             Link(Link::Url, channel.name() + "\n" + m_id)));
    }

    // username
    m_userName = ircMessage.account();

    if (m_userName.isEmpty()) {
        auto iterator = tags.find("login");

        if (iterator != tags.end()) {
            m_userName = iterator.value().toString();
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
        userDisplayString = IrcManager::account->username() + " -> ";
    }

    if (isReceivedWhisper) {
        userDisplayString += " -> " + IrcManager::account->username();
    }

    if (!ircMessage.isAction()) {
        userDisplayString += ": ";
    }

    words.push_back(Word(userDisplayString, Word::Username, usernameColor,
                         userDisplayString, QString()));

// highlights
#pragma message WARN("xD")

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

                QString name = ircMessage.content().mid(start, end - start);

                twitchEmotes.push_back(std::pair<long int, LazyLoadedImage *>(
                    start, Emotes::getTwitchEmoteById(name, id)));
            }
        }

        std::sort(twitchEmotes.begin(), twitchEmotes.end(), sortTwitchEmotes);
    }

    auto currentTwitchEmote = twitchEmotes.begin();

    // words
    QColor textColor =
        ircMessage.isAction() ? usernameColor : ColorScheme::instance().Text;

    QStringList splits = ircMessage.content().split(' ');

    long int i = 0;

    for (QString split : splits) {
        // twitch emote
        if (currentTwitchEmote == twitchEmotes.end())
            break;

        if (currentTwitchEmote->first == i) {
            words.push_back(Word(currentTwitchEmote->second,
                                 Word::TwitchEmoteImage,
                                 currentTwitchEmote->second->name(),
                                 currentTwitchEmote->second->name() +
                                     QString("\nTwitch Emote")));
            words.push_back(Word(currentTwitchEmote->second->name(),
                                 Word::TwitchEmoteText, textColor,
                                 currentTwitchEmote->second->name(),
                                 currentTwitchEmote->second->name() +
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
            if (image == NULL) {
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
                        Emotes::miscImageFromCache().getOrAdd(
                            bitsLinkAnimated, [&bitsLinkAnimated] {
                                return new LazyLoadedImage(bitsLinkAnimated);
                            });
                    LazyLoadedImage *image =
                        Emotes::miscImageFromCache().getOrAdd(
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
                        Word(image, Word::Bits, QString("cheer"),
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

#pragma message WARN("xD ignored emotes")
                if (Emotes::bttvEmotes().tryGet(string, bttvEmote) ||
                    channel.bttvChannelEmotes().tryGet(string, bttvEmote) ||
                    Emotes::ffzEmotes().tryGet(string, bttvEmote) ||
                    channel.ffzChannelEmotes().tryGet(string, bttvEmote) ||
                    Emotes::chatterinoEmotes().tryGet(string, bttvEmote)) {
                    words.push_back(Word(bttvEmote, Word::BttvEmoteImage,
                                         bttvEmote->name(),
                                         bttvEmote->tooltip(),
                                         Link(Link::Url, bttvEmote->url())));

                    continue;
                }

                // actually just a word
                QString link = matchLink(string);

                words.push_back(
                    Word(string, Word::Text, textColor, string, QString(),
                         link.isEmpty() ? Link() : Link(Link::Url, link)));
            }
        }

        i += split.length() + 1;
    }

    this->m_words = words;

#pragma message WARN("xD")
    //    if (!isReceivedWhisper &&
    //    AppSettings.HighlightIgnoredUsers.ContainsKey(Username))
    //    {
    //        HighlightTab = false;
    //    }
}

bool
Message::layout(int width, bool enableEmoteMargins)
{
    width = width - (width % 2);

    int mediumTextLineHeight = Fonts::getFontMetrics(Fonts::Medium).height();
    int spaceWidth = 4;

    bool redraw = width != m_currentLayoutWidth || m_relayoutRequested;

    if (m_recalculateImages || m_recalculateText) {
        redraw = true;

        for (auto &word : m_words) {
            if (word.isImage()) {
                if (m_recalculateImages) {
                    auto &image = word.getImage();

                    qreal w = image.width();
                    qreal h = image.height();

                    if (AppSettings::scaleEmotesByLineHeight()) {
                        word.setSize(
                            w * mediumTextLineHeight / h *
                                AppSettings::emoteScale(),
                            mediumTextLineHeight * AppSettings::emoteScale());
                    } else {
                        word.setSize(
                            w * image.scale() * AppSettings::emoteScale(),
                            h * image.scale() * AppSettings::emoteScale());
                    }
                }
            } else {
                if (m_recalculateText) {
                    QFontMetrics &metrics = word.getFontMetrics();
                    word.setSize(metrics.width(word.getText()),
                                 metrics.height());
                }
            }
        }

        m_recalculateImages = false;
        m_recalculateText = false;
    }

    if (!redraw) {
        return false;
    }

    int x = MARGIN_LEFT;
    int y = MARGIN_TOP;

    int right = width - MARGIN_RIGHT - MARGIN_LEFT;

    std::list<WordPart> *parts = new std::list<WordPart>();

    auto lineStart = parts->begin();
    int lineHeight = 0;

    auto alignParts = [&lineStart, &lineHeight, &parts, this] {
        for (auto it2 = lineStart; true; it2++) {
            WordPart &wordPart2 = *it2;

            wordPart2.setY(wordPart2.y() + lineHeight);

            if (it2 == parts->end()) {
                break;
            }
        }
    };

    int flags = AppSettings::wordTypeMask();

    for (auto it = m_words.begin(); it != m_words.end(); ++it) {
        Word &word = *it;

        if ((word.type() & flags) == Word::None) {
            continue;
        }

        int xOffset = 0, yOffset = 0;

        if (enableEmoteMargins) {
            if (word.isImage() && word.getImage().isHat()) {
                xOffset = -word.width() + 2;
            } else {
                xOffset = word.xOffset();
                yOffset = word.yOffset();
            }
        }

        // fits in the line
        if (x + word.width() + xOffset <= right) {
            parts->push_back(
                WordPart(word, x, y - word.height(), word.copyText()));

            x += word.width() + xOffset;

            if (word.hasTrailingSpace()) {
                x += spaceWidth;
            }

            lineHeight = std::max(word.height(), lineHeight);
        }

        //            else if (word.isText() && word.getText().length()
        //            > 2)
        //            {
        //            }

        // doesn't fit in the line
        else {
            alignParts();

            y += lineHeight;

            parts->push_back(WordPart(word, MARGIN_LEFT, y - word.height(),
                                      word.copyText()));

            lineStart = parts->end();

            lineHeight = word.height();

            x = word.width();

            if (word.hasTrailingSpace()) {
                x += spaceWidth;
            }
        }
    }

    alignParts();

    qInfo("words: %d, parts: %d", m_words.size(), parts->size());

    auto tmp = m_wordParts;
    m_wordParts = parts;
    delete tmp;

    m_height = y + lineHeight;

    return true;
}

bool
Message::sortTwitchEmotes(const std::pair<long int, LazyLoadedImage *> &a,
                          const std::pair<long int, LazyLoadedImage *> &b)
{
    return a.first < b.first;
}

QString
Message::matchLink(const QString &string)
{
#pragma message WARN("xD")
    return QString();
}
