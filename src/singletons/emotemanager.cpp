#include "emotemanager.hpp"

#include "application.hpp"
#include "common.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/windowmanager.hpp"
#include "util/urlfetch.hpp"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <memory>

using namespace chatterino::providers::twitch;
using namespace chatterino::messages;

namespace chatterino {
namespace singletons {

namespace {

QString GetBTTVEmoteLink(QString urlTemplate, const QString &id, const QString &emoteScale)
{
    urlTemplate.detach();

    return urlTemplate.replace("{{id}}", id).replace("{{image}}", emoteScale);
}

QString GetFFZEmoteLink(const QJsonObject &urls, const QString &emoteScale)
{
    auto emote = urls.value(emoteScale);
    if (emote.isUndefined()) {
        return "";
    }

    assert(emote.isString());

    return "https:" + emote.toString();
}

void FillInFFZEmoteData(const QJsonObject &urls, const QString &code, const QString &tooltip,
                        util::EmoteData &emoteData)
{
    QString url1x = GetFFZEmoteLink(urls, "1");
    QString url2x = GetFFZEmoteLink(urls, "2");
    QString url3x = GetFFZEmoteLink(urls, "4");

    assert(!url1x.isEmpty());

    emoteData.image1x = new Image(url1x, 1, code, tooltip);

    if (!url2x.isEmpty()) {
        emoteData.image2x = new Image(url2x, 0.5, code, tooltip);
    }

    if (!url3x.isEmpty()) {
        emoteData.image3x = new Image(url3x, 0.25, code, tooltip);
    }
}

}  // namespace

EmoteManager::EmoteManager()
    : findShortCodesRegex(":([-+\\w]+):")
{
    qDebug() << "init EmoteManager";
}

void EmoteManager::initialize()
{
    getApp()->accounts->twitch.currentUserChanged.connect([this] {
        auto currentUser = getApp()->accounts->twitch.getCurrent();
        assert(currentUser);
        this->twitch.refresh(currentUser);
    });

    this->loadEmojis();
    this->loadBTTVEmotes();
    this->loadFFZEmotes();
}

void EmoteManager::reloadBTTVChannelEmotes(const QString &channelName,
                                           std::weak_ptr<util::EmoteMap> _map)
{
    printf("[EmoteManager] Reload BTTV Channel Emotes for channel %s\n", qPrintable(channelName));

    QString url("https://api.betterttv.net/2/channels/" + channelName);

    debug::Log("Request bttv channel emotes for {}", channelName);

    util::NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.setTimeout(3000);
    req.getJSON([this, channelName, _map](QJsonObject &rootNode) {
        auto map = _map.lock();

        if (_map.expired()) {
            return;
        }

        map->clear();

        auto emotesNode = rootNode.value("emotes").toArray();

        QString linkTemplate = "https:" + rootNode.value("urlTemplate").toString();

        std::vector<std::string> codes;
        for (const QJsonValue &emoteNode : emotesNode) {
            QJsonObject emoteObject = emoteNode.toObject();

            QString id = emoteObject.value("id").toString();
            QString code = emoteObject.value("code").toString();
            // emoteObject.value("imageType").toString();

            auto emote = this->getBTTVChannelEmoteFromCaches().getOrAdd(id, [&] {
                util::EmoteData emoteData;
                QString link = linkTemplate;
                link.detach();
                emoteData.image1x = new Image(link.replace("{{id}}", id).replace("{{image}}", "1x"),
                                              1, code, code + "<br />Channel BTTV Emote");
                link = linkTemplate;
                link.detach();
                emoteData.image2x = new Image(link.replace("{{id}}", id).replace("{{image}}", "2x"),
                                              0.5, code, code + "<br />Channel BTTV Emote");
                link = linkTemplate;
                link.detach();
                emoteData.image3x = new Image(link.replace("{{id}}", id).replace("{{image}}", "3x"),
                                              0.25, code, code + "<br />Channel BTTV Emote");
                emoteData.pageLink = "https://manage.betterttv.net/emotes/" + id;

                return emoteData;
            });

            this->bttvChannelEmotes.insert(code, emote);
            map->insert(code, emote);
            codes.push_back(code.toStdString());
        }

        this->bttvChannelEmoteCodes[channelName.toStdString()] = codes;
    });
}

void EmoteManager::reloadFFZChannelEmotes(const QString &channelName,
                                          std::weak_ptr<util::EmoteMap> _map)
{
    printf("[EmoteManager] Reload FFZ Channel Emotes for channel %s\n", qPrintable(channelName));

    QString url("https://api.frankerfacez.com/v1/room/" + channelName);

    util::NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.setTimeout(3000);
    req.getJSON([this, channelName, _map](QJsonObject &rootNode) {
        auto map = _map.lock();

        if (_map.expired()) {
            return;
        }

        map->clear();

        auto setsNode = rootNode.value("sets").toObject();

        std::vector<std::string> codes;
        for (const QJsonValue &setNode : setsNode) {
            auto emotesNode = setNode.toObject().value("emoticons").toArray();

            for (const QJsonValue &emoteNode : emotesNode) {
                QJsonObject emoteObject = emoteNode.toObject();

                // margins
                int id = emoteObject.value("id").toInt();
                QString code = emoteObject.value("name").toString();

                QJsonObject urls = emoteObject.value("urls").toObject();

                auto emote = this->getFFZChannelEmoteFromCaches().getOrAdd(id, [id, &code, &urls] {
                    util::EmoteData emoteData;
                    FillInFFZEmoteData(urls, code, code + "<br/>Channel FFZ Emote", emoteData);
                    emoteData.pageLink =
                        QString("https://www.frankerfacez.com/emoticon/%1-%2").arg(id).arg(code);

                    return emoteData;
                });

                this->ffzChannelEmotes.insert(code, emote);
                map->insert(code, emote);
                codes.push_back(code.toStdString());
            }

            this->ffzChannelEmoteCodes[channelName.toStdString()] = codes;
        }
    });
}

util::EmoteMap &EmoteManager::getFFZEmotes()
{
    return ffzGlobalEmotes;
}

util::EmoteMap &EmoteManager::getChatterinoEmotes()
{
    return _chatterinoEmotes;
}

util::EmoteMap &EmoteManager::getBTTVChannelEmoteFromCaches()
{
    return _bttvChannelEmoteFromCaches;
}

util::EmojiMap &EmoteManager::getEmojis()
{
    return this->emojis;
}

util::ConcurrentMap<int, util::EmoteData> &EmoteManager::getFFZChannelEmoteFromCaches()
{
    return _ffzChannelEmoteFromCaches;
}

void EmoteManager::loadEmojis()
{
    QFile file(":/emojidata.txt");
    file.open(QFile::ReadOnly);
    QTextStream in(&file);

    uint unicodeBytes[4];

    while (!in.atEnd()) {
        // Line example: sunglasses 1f60e
        QString line = in.readLine();

        if (line.at(0) == '#') {
            // Ignore lines starting with # (comments)
            continue;
        }

        QStringList parts = line.split(' ');
        if (parts.length() < 2) {
            continue;
        }

        QString shortCode = parts[0];
        QString code = parts[1];

        QStringList unicodeCharacters = code.split('-');
        if (unicodeCharacters.length() < 1) {
            continue;
        }

        int numUnicodeBytes = 0;

        for (const QString &unicodeCharacter : unicodeCharacters) {
            unicodeBytes[numUnicodeBytes++] = QString(unicodeCharacter).toUInt(nullptr, 16);
        }

        QString unicodeString = QString::fromUcs4(unicodeBytes, numUnicodeBytes);

        QString url = "https://cdnjs.cloudflare.com/ajax/libs/"
                      "emojione/2.2.6/assets/png/" +
                      code + ".png";

        EmojiData emojiData{
            unicodeString,  //
            code,           //
            shortCode,      //
            {new Image(url, 0.35, unicodeString, ":" + shortCode + ":<br/>Emoji")},
        };

        this->emojiShortCodeToEmoji.insert(shortCode, emojiData);
        this->emojiShortCodes.push_back(shortCode.toStdString());

        this->emojiFirstByte[emojiData.value.at(0)].append(emojiData);

        this->emojis.insert(code, emojiData);
    }

    for (auto &p : this->emojiFirstByte) {
        std::stable_sort(p.begin(), p.end(), [](const auto &lhs, const auto &rhs) {
            return lhs.value.length() > rhs.value.length();
        });
    }
}

void EmoteManager::parseEmojis(std::vector<std::tuple<util::EmoteData, QString>> &parsedWords,
                               const QString &text)
{
    int lastParsedEmojiEndIndex = 0;

    for (auto i = 0; i < text.length(); ++i) {
        const QChar character = text.at(i);

        if (character.isLowSurrogate()) {
            continue;
        }

        auto it = this->emojiFirstByte.find(character);
        if (it == this->emojiFirstByte.end()) {
            // No emoji starts with this character
            continue;
        }

        const QVector<EmojiData> possibleEmojis = it.value();

        int remainingCharacters = text.length() - i - 1;

        EmojiData matchedEmoji;

        int matchedEmojiLength = 0;

        for (const EmojiData &emoji : possibleEmojis) {
            int emojiExtraCharacters = emoji.value.length() - 1;
            if (emojiExtraCharacters > remainingCharacters) {
                // It cannot be this emoji, there's not enough space for it
                continue;
            }

            bool match = true;

            for (int j = 1; j < emoji.value.length(); ++j) {
                if (text.at(i + j) != emoji.value.at(j)) {
                    match = false;

                    break;
                }
            }

            if (match) {
                matchedEmoji = emoji;
                matchedEmojiLength = emoji.value.length();

                break;
            }
        }

        if (matchedEmojiLength == 0) {
            continue;
        }

        int currentParsedEmojiFirstIndex = i;
        int currentParsedEmojiEndIndex = i + (matchedEmojiLength);

        int charactersFromLastParsedEmoji = currentParsedEmojiFirstIndex - lastParsedEmojiEndIndex;

        if (charactersFromLastParsedEmoji > 0) {
            // Add characters inbetween emojis
            parsedWords.emplace_back(util::EmoteData(), text.mid(lastParsedEmojiEndIndex,
                                                                 charactersFromLastParsedEmoji));
        }

        // Push the emoji as a word to parsedWords
        parsedWords.push_back(
            std::tuple<util::EmoteData, QString>(matchedEmoji.emoteData, QString()));

        lastParsedEmojiEndIndex = currentParsedEmojiEndIndex;

        i += matchedEmojiLength - 1;
    }

    if (lastParsedEmojiEndIndex < text.length()) {
        // Add remaining characters
        parsedWords.emplace_back(util::EmoteData(), text.mid(lastParsedEmojiEndIndex));
    }
}

QString EmoteManager::replaceShortCodes(const QString &text)
{
    QString ret(text);
    auto it = this->findShortCodesRegex.globalMatch(text);

    int32_t offset = 0;

    while (it.hasNext()) {
        auto match = it.next();

        auto capturedString = match.captured();

        QString matchString = capturedString.toLower().mid(1, capturedString.size() - 2);

        auto emojiIt = this->emojiShortCodeToEmoji.constFind(matchString);

        if (emojiIt == this->emojiShortCodeToEmoji.constEnd()) {
            continue;
        }

        auto emojiData = emojiIt.value();

        ret.replace(offset + match.capturedStart(), match.capturedLength(), emojiData.value);

        offset += emojiData.value.size() - match.capturedLength();
    }

    return ret;
}

void EmoteManager::loadBTTVEmotes()
{
    QString url("https://api.betterttv.net/2/emotes");

    util::NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.setTimeout(30000);
    req.setUseQuickLoadCache(true);
    req.getJSON([this](QJsonObject &root) {
        auto emotes = root.value("emotes").toArray();

        QString urlTemplate = "https:" + root.value("urlTemplate").toString();

        std::vector<std::string> codes;
        for (const QJsonValue &emote : emotes) {
            QString id = emote.toObject().value("id").toString();
            QString code = emote.toObject().value("code").toString();

            util::EmoteData emoteData;
            emoteData.image1x = new Image(GetBTTVEmoteLink(urlTemplate, id, "1x"), 1, code,
                                          code + "<br />Global BTTV Emote");
            emoteData.image2x = new Image(GetBTTVEmoteLink(urlTemplate, id, "2x"), 0.5, code,
                                          code + "<br />Global BTTV Emote");
            emoteData.image3x = new Image(GetBTTVEmoteLink(urlTemplate, id, "3x"), 0.25, code,
                                          code + "<br />Global BTTV Emote");
            emoteData.pageLink = "https://manage.betterttv.net/emotes/" + id;

            this->bttvGlobalEmotes.insert(code, emoteData);
            codes.push_back(code.toStdString());
        }

        this->bttvGlobalEmoteCodes = codes;
    });
}

void EmoteManager::loadFFZEmotes()
{
    QString url("https://api.frankerfacez.com/v1/set/global");

    util::NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.setTimeout(30000);
    req.getJSON([this](QJsonObject &root) {
        auto sets = root.value("sets").toObject();

        std::vector<std::string> codes;
        for (const QJsonValue &set : sets) {
            auto emoticons = set.toObject().value("emoticons").toArray();

            for (const QJsonValue &emote : emoticons) {
                QJsonObject object = emote.toObject();

                QString code = object.value("name").toString();
                int id = object.value("id").toInt();
                QJsonObject urls = object.value("urls").toObject();

                util::EmoteData emoteData;
                FillInFFZEmoteData(urls, code, code + "<br/>Global FFZ Emote", emoteData);
                emoteData.pageLink =
                    QString("https://www.frankerfacez.com/emoticon/%1-%2").arg(id).arg(code);

                this->ffzGlobalEmotes.insert(code, emoteData);
                codes.push_back(code.toStdString());
            }

            this->ffzGlobalEmoteCodes = codes;
        }
    });
}

util::EmoteData EmoteManager::getCheerImage(long long amount, bool animated)
{
    // TODO: fix this xD
    return util::EmoteData();
}

pajlada::Signals::NoArgSignal &EmoteManager::getGifUpdateSignal()
{
    if (!this->gifUpdateTimerInitiated) {
        auto app = getApp();

        this->gifUpdateTimerInitiated = true;

        this->gifUpdateTimer.setInterval(30);
        this->gifUpdateTimer.start();

        app->settings->enableGifAnimations.connect([this](bool enabled, auto) {
            if (enabled) {
                this->gifUpdateTimer.start();
            } else {
                this->gifUpdateTimer.stop();
            }
        });

        QObject::connect(&this->gifUpdateTimer, &QTimer::timeout, [this] {
            this->gifUpdateTimerSignal.invoke();
            // fourtf:
            auto app = getApp();
            app->windows->repaintGifEmotes();
        });
    }

    return this->gifUpdateTimerSignal;
}

}  // namespace singletons
}  // namespace chatterino

#if 0
namespace chatterino {

void EmojiTest()
{
    auto &emoteManager = singletons::EmoteManager::getInstance();

    emoteManager.loadEmojis();

    {
        std::vector<std::tuple<util::EmoteData, QString>> dummy;

        // couple_mm 1f468-2764-1f468
        // "\154075\156150❤\154075\156150"
        // [0]            55357    0xd83d    QChar
        // [1]            56424    0xdc68    QChar
        // [2]    '❤'     10084    0x2764    QChar
        // [3]            55357    0xd83d    QChar
        // [4]            56424    0xdc68    QChar
        QString text = "👨❤👨";

        emoteManager.parseEmojis(dummy, text);

        assert(dummy.size() == 1);
    }

    {
        std::vector<std::tuple<util::EmoteData, QString>> dummy;

        // "✍\154074\157777"
        // [0]    '✍'     9997    0x270d    QChar
        // [1]            55356    0xd83c    QChar
        // [2]            57343    0xdfff    QChar
        QString text = "✍🏿";

        emoteManager.parseEmojis(dummy, text);

        assert(dummy.size() == 1);

        assert(std::get<0>(dummy[0]).isValid());
    }

    {
        std::vector<std::tuple<util::EmoteData, QString>> dummy;

        QString text = "✍";

        emoteManager.parseEmojis(dummy, text);

        assert(dummy.size() == 1);

        assert(std::get<0>(dummy[0]).isValid());
    }
}

}  // namespace chatterino
#endif
