#include "emotemanager.hpp"
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

#define TWITCH_EMOTE_TEMPLATE "https://static-cdn.jtvnw.net/emoticons/v1/{id}/{scale}"

using namespace chatterino::providers::twitch;
using namespace chatterino::messages;

namespace chatterino {
namespace singletons {

namespace {

static QString GetTwitchEmoteLink(long id, const QString &emoteScale)
{
    QString value = TWITCH_EMOTE_TEMPLATE;

    value.detach();

    return value.replace("{id}", QString::number(id)).replace("{scale}", emoteScale);
}

static QString GetBTTVEmoteLink(QString urlTemplate, const QString &id, const QString &emoteScale)
{
    urlTemplate.detach();

    return urlTemplate.replace("{{id}}", id).replace("{{image}}", emoteScale);
}

static QString GetFFZEmoteLink(const QJsonObject &urls, const QString &emoteScale)
{
    auto emote = urls.value(emoteScale);
    if (emote.isUndefined()) {
        return "";
    }

    assert(emote.isString());

    return "http:" + emote.toString();
}

static void FillInFFZEmoteData(const QJsonObject &urls, const QString &code,
                               util::EmoteData &emoteData)
{
    QString url1x = GetFFZEmoteLink(urls, "1");
    QString url2x = GetFFZEmoteLink(urls, "2");
    QString url3x = GetFFZEmoteLink(urls, "4");

    assert(!url1x.isEmpty());

    emoteData.image1x = new Image(url1x, 1, code, code);

    if (!url2x.isEmpty()) {
        emoteData.image2x = new Image(url2x, 0.5, code, code);
    }

    if (!url3x.isEmpty()) {
        emoteData.image3x = new Image(url3x, 0.25, code, code);
    }
}

}  // namespace

EmoteManager::EmoteManager(SettingManager &_settingsManager, WindowManager &_windowManager)
    : settingsManager(_settingsManager)
    , windowManager(_windowManager)
    , findShortCodesRegex(":([-+\\w]+):")
{
    auto &accountManager = AccountManager::getInstance();

    accountManager.Twitch.userChanged.connect([this] {
        auto currentUser = AccountManager::getInstance().Twitch.getCurrent();
        assert(currentUser);
        this->refreshTwitchEmotes(currentUser);
    });
}

EmoteManager &EmoteManager::getInstance()
{
    static EmoteManager instance(SettingManager::getInstance(), WindowManager::getInstance());
    return instance;
}

void EmoteManager::loadGlobalEmotes()
{
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
        debug::Log("Got bttv channel emotes for {}", channelName);
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

            QString link = linkTemplate;
            link.detach();

            link = link.replace("{{id}}", id).replace("{{image}}", "1x");

            auto emote = this->getBTTVChannelEmoteFromCaches().getOrAdd(id, [this, &code, &link] {
                return util::EmoteData(new Image(link, 1, code, code + "<br/>Channel BTTV Emote"));
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

    QString url("http://api.frankerfacez.com/v1/room/" + channelName);

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

                auto emote =
                    this->getFFZChannelEmoteFromCaches().getOrAdd(id, [this, &code, &urls] {
                        util::EmoteData emoteData;
                        FillInFFZEmoteData(urls, code + "<br/>Channel FFZ Emote", emoteData);

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

util::ConcurrentMap<QString, providers::twitch::EmoteValue *> &EmoteManager::getTwitchEmotes()
{
    return _twitchEmotes;
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

util::EmoteMap &EmoteManager::getEmojis()
{
    return this->emojis;
}

util::ConcurrentMap<int, util::EmoteData> &EmoteManager::getFFZChannelEmoteFromCaches()
{
    return _ffzChannelEmoteFromCaches;
}

util::ConcurrentMap<long, util::EmoteData> &EmoteManager::getTwitchEmoteFromCache()
{
    return _twitchEmoteFromCache;
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

        EmojiData emojiData{
            QString::fromUcs4(unicodeBytes, numUnicodeBytes),  //
            code,                                              //
            shortCode,                                         //
        };

        this->emojiShortCodeToEmoji.insert(shortCode, emojiData);
        this->emojiShortCodes.push_back(shortCode.toStdString());

        this->emojiFirstByte[emojiData.value.at(0)].append(emojiData);

        QString url = "https://cdnjs.cloudflare.com/ajax/libs/"
                      "emojione/2.2.6/assets/png/" +
                      code + ".png";

        this->emojis.insert(code, util::EmoteData(new Image(url, 0.35, ":" + shortCode + ":",
                                                            ":" + shortCode + ":<br/>Emoji")));

        // TODO(pajlada): The vectors in emojiFirstByte need to be sorted by
        // emojiData.code.length()
    }
}

void EmoteManager::parseEmojis(std::vector<std::tuple<util::EmoteData, QString>> &parsedWords,
                               const QString &text)
{
    int lastParsedEmojiEndIndex = 0;

    for (auto i = 0; i < text.length() - 1; i++) {
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

        int remainingCharacters = text.length() - i;

        EmojiData matchedEmoji;

        int matchedEmojiLength = 0;

        for (const EmojiData &emoji : possibleEmojis) {
            if (remainingCharacters < emoji.value.length()) {
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
            parsedWords.push_back(std::tuple<util::EmoteData, QString>(
                util::EmoteData(),
                text.mid(lastParsedEmojiEndIndex, charactersFromLastParsedEmoji)));
        }

        QString url = "https://cdnjs.cloudflare.com/ajax/libs/"
                      "emojione/2.2.6/assets/png/" +
                      matchedEmoji.code + ".png";

        // Create or fetch cached emoji image
        auto emojiImage = this->emojis.getOrAdd(matchedEmoji.code, [this, &url] {
            return util::EmoteData(new Image(url, 0.35, "?????????", "???????????????"));  //
        });

        // Push the emoji as a word to parsedWords
        parsedWords.push_back(std::tuple<util::EmoteData, QString>(emojiImage, QString()));

        lastParsedEmojiEndIndex = currentParsedEmojiEndIndex;

        i += matchedEmojiLength - 1;
    }

    if (lastParsedEmojiEndIndex < text.length()) {
        // Add remaining characters
        parsedWords.push_back(std::tuple<util::EmoteData, QString>(
            util::EmoteData(), text.mid(lastParsedEmojiEndIndex)));
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

void EmoteManager::refreshTwitchEmotes(const std::shared_ptr<TwitchAccount> &user)
{
    debug::Log("Loading Twitch emotes for user {}", user->getUserName());

    const auto &roomID = user->getUserId();
    const auto &clientID = user->getOAuthClient();
    const auto &oauthToken = user->getOAuthToken();

    if (clientID.isEmpty() || oauthToken.isEmpty()) {
        debug::Log("Missing Client ID or OAuth token");
        return;
    }

    TwitchAccountEmoteData &emoteData = this->twitchAccountEmotes[roomID.toStdString()];

    if (emoteData.filled) {
        qDebug() << "Already loaded for room id " << roomID;
        return;
    }

    QString url("https://api.twitch.tv/kraken/users/" + roomID + "/emotes");

    util::twitch::getAuthorized(
        url, clientID, oauthToken, QThread::currentThread(),
        [=, &emoteData](const QJsonObject &root) {
            emoteData.emoteSets.clear();
            emoteData.emoteCodes.clear();
            auto emoticonSets = root.value("emoticon_sets").toObject();
            for (QJsonObject::iterator it = emoticonSets.begin(); it != emoticonSets.end(); ++it) {
                std::string emoteSetString = it.key().toStdString();
                QJsonArray emoteSetList = it.value().toArray();

                for (QJsonValue emoteValue : emoteSetList) {
                    QJsonObject emoticon = emoteValue.toObject();
                    std::string id = emoticon["id"].toString().toStdString();
                    std::string code = emoticon["code"].toString().toStdString();
                    emoteData.emoteSets[emoteSetString].push_back({id, code});
                    emoteData.emoteCodes.push_back(code);
                }
            }

            emoteData.filled = true;
        });
}

void EmoteManager::loadBTTVEmotes()
{
    QString url("https://api.betterttv.net/2/emotes");

    util::NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.setTimeout(30000);
    req.setUseQuickLoadCache(true);
    req.getJSON([this](QJsonObject &root) {
        debug::Log("Got global bttv emotes");
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
        debug::Log("Got global ffz emotes");

        auto sets = root.value("sets").toObject();

        std::vector<std::string> codes;
        for (const QJsonValue &set : sets) {
            auto emoticons = set.toObject().value("emoticons").toArray();

            for (const QJsonValue &emote : emoticons) {
                QJsonObject object = emote.toObject();

                QString code = object.value("name").toString();
                QJsonObject urls = object.value("urls").toObject();

                util::EmoteData emoteData;
                FillInFFZEmoteData(urls, code + "<br/>Global FFZ Emote", emoteData);

                this->ffzGlobalEmotes.insert(code, emoteData);
                codes.push_back(code.toStdString());
            }

            this->ffzGlobalEmoteCodes = codes;
        }
    });
}

// id is used for lookup
// emoteName is used for giving a name to the emote in case it doesn't exist
util::EmoteData EmoteManager::getTwitchEmoteById(long id, const QString &emoteName)
{
    QString _emoteName = emoteName;
    _emoteName.replace("<", "&lt;");

    return _twitchEmoteFromCache.getOrAdd(id, [this, &emoteName, &_emoteName, &id] {
        util::EmoteData newEmoteData;
        newEmoteData.image1x = new Image(GetTwitchEmoteLink(id, "1.0"), 1, emoteName,
                                         _emoteName + "<br/>Twitch Emote 1x");
        newEmoteData.image2x = new Image(GetTwitchEmoteLink(id, "2.0"), .5, emoteName,
                                         _emoteName + "<br/>Twitch Emote 2x");
        newEmoteData.image3x = new Image(GetTwitchEmoteLink(id, "3.0"), .25, emoteName,
                                         _emoteName + "<br/>Twitch Emote 3x");

        return newEmoteData;
    });
}

util::EmoteData EmoteManager::getCheerImage(long long amount, bool animated)
{
    // TODO: fix this xD
    return util::EmoteData();
}

boost::signals2::signal<void()> &EmoteManager::getGifUpdateSignal()
{
    if (!this->gifUpdateTimerInitiated) {
        this->gifUpdateTimerInitiated = true;

        this->gifUpdateTimer.setInterval(30);
        this->gifUpdateTimer.start();

        this->settingsManager.enableGifAnimations.connect([this](bool enabled, auto) {
            if (enabled) {
                this->gifUpdateTimer.start();
            } else {
                this->gifUpdateTimer.stop();
            }
        });

        QObject::connect(&this->gifUpdateTimer, &QTimer::timeout, [this] {
            this->gifUpdateTimerSignal();
            // fourtf:
            this->windowManager.repaintGifEmotes();
        });
    }

    return this->gifUpdateTimerSignal;
}

}  // namespace singletons
}  // namespace chatterino
