#include "providers/emoji/Emojis.hpp"

#include "Application.hpp"
#include "debug/Log.hpp"
#include "singletons/Settings.hpp"

#include <rapidjson/error/en.h>
#include <rapidjson/error/error.h>
#include <rapidjson/rapidjson.h>
#include <QFile>
#include <boost/variant.hpp>
#include <memory>

namespace chatterino {

namespace {

void parseEmoji(const std::shared_ptr<EmojiData> &emojiData, const rapidjson::Value &unparsedEmoji,
                QString shortCode = QString())
{
    static uint unicodeBytes[4];

    struct {
        bool apple;
        bool google;
        bool twitter;
        bool emojione;
        bool facebook;
        bool messenger;
    } capabilities;

    if (!shortCode.isEmpty()) {
        emojiData->shortCodes.push_back(shortCode);
    } else {
        const auto &shortCodes = unparsedEmoji["short_names"];
        for (const auto &shortCode : shortCodes.GetArray()) {
            emojiData->shortCodes.emplace_back(shortCode.GetString());
        }
    }

    rj::getSafe(unparsedEmoji, "non_qualified", emojiData->nonQualifiedCode);
    rj::getSafe(unparsedEmoji, "unified", emojiData->unifiedCode);

    rj::getSafe(unparsedEmoji, "has_img_apple", capabilities.apple);
    rj::getSafe(unparsedEmoji, "has_img_google", capabilities.google);
    rj::getSafe(unparsedEmoji, "has_img_twitter", capabilities.twitter);
    rj::getSafe(unparsedEmoji, "has_img_emojione", capabilities.emojione);
    rj::getSafe(unparsedEmoji, "has_img_facebook", capabilities.facebook);
    rj::getSafe(unparsedEmoji, "has_img_messenger", capabilities.messenger);

    if (capabilities.apple) {
        emojiData->capabilities.insert("Apple");
    }
    if (capabilities.google) {
        emojiData->capabilities.insert("Google");
    }
    if (capabilities.twitter) {
        emojiData->capabilities.insert("Twitter");
    }
    if (capabilities.emojione) {
        emojiData->capabilities.insert("EmojiOne 3");
    }
    if (capabilities.facebook) {
        emojiData->capabilities.insert("Facebook");
    }
    if (capabilities.messenger) {
        emojiData->capabilities.insert("Messenger");
    }

    QStringList unicodeCharacters;
    if (!emojiData->nonQualifiedCode.isEmpty()) {
        unicodeCharacters = emojiData->nonQualifiedCode.toLower().split('-');
    } else {
        unicodeCharacters = emojiData->unifiedCode.toLower().split('-');
    }
    if (unicodeCharacters.length() < 1) {
        return;
    }

    int numUnicodeBytes = 0;

    for (const QString &unicodeCharacter : unicodeCharacters) {
        unicodeBytes[numUnicodeBytes++] = QString(unicodeCharacter).toUInt(nullptr, 16);
    }

    emojiData->value = QString::fromUcs4(unicodeBytes, numUnicodeBytes);
}

}  // namespace

void Emojis::load()
{
    this->loadEmojis();

    this->loadEmojiOne2Capabilities();

    this->sortEmojis();

    this->loadEmojiSet();
}

void Emojis::loadEmojis()
{
    std::map<std::string, QString> toneNames;
    toneNames["1F3FB"] = "tone1";
    toneNames["1F3FC"] = "tone2";
    toneNames["1F3FD"] = "tone3";
    toneNames["1F3FE"] = "tone4";
    toneNames["1F3FF"] = "tone5";

    QFile file(":/emoji.json");
    file.open(QFile::ReadOnly);
    QTextStream s1(&file);
    QString data = s1.readAll();
    rapidjson::Document root;
    rapidjson::ParseResult result = root.Parse(data.toUtf8(), data.length());

    if (result.Code() != rapidjson::kParseErrorNone) {
        Log("JSON parse error: {} ({})", rapidjson::GetParseError_En(result.Code()),
            result.Offset());
        return;
    }

    for (const auto &unparsedEmoji : root.GetArray()) {
        auto emojiData = std::make_shared<EmojiData>();
        parseEmoji(emojiData, unparsedEmoji);

        for (const auto &shortCode : emojiData->shortCodes) {
            this->emojiShortCodeToEmoji_.insert(shortCode, emojiData);
            this->shortCodes.emplace_back(shortCode);
        }

        this->emojiFirstByte_[emojiData->value.at(0)].append(emojiData);

        this->emojis.insert(emojiData->unifiedCode, emojiData);

        if (unparsedEmoji.HasMember("skin_variations")) {
            for (const auto &skinVariation : unparsedEmoji["skin_variations"].GetObject()) {
                std::string tone = skinVariation.name.GetString();
                const auto &variation = skinVariation.value;

                auto variationEmojiData = std::make_shared<EmojiData>();

                auto toneNameIt = toneNames.find(tone);
                if (toneNameIt == toneNames.end()) {
                    Log("Tone with key {} does not exist in tone names map", tone);
                    continue;
                }

                parseEmoji(variationEmojiData, variation,
                           emojiData->shortCodes[0] + "_" + toneNameIt->second);

                this->emojiShortCodeToEmoji_.insert(variationEmojiData->shortCodes[0],
                                                    variationEmojiData);
                this->shortCodes.push_back(variationEmojiData->shortCodes[0]);

                this->emojiFirstByte_[variationEmojiData->value.at(0)].append(variationEmojiData);

                this->emojis.insert(variationEmojiData->unifiedCode, variationEmojiData);
            }
        }
    }
}

void Emojis::loadEmojiOne2Capabilities()
{
    QFile file(":/emojidata.txt");
    file.open(QFile::ReadOnly);
    QTextStream in(&file);

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

        auto emojiIt = this->emojiShortCodeToEmoji_.find(shortCode);
        if (emojiIt != this->emojiShortCodeToEmoji_.end()) {
            std::shared_ptr<EmojiData> emoji = *emojiIt;
            emoji->capabilities.insert("EmojiOne 2");
            continue;
        }
    }
}

void Emojis::sortEmojis()
{
    for (auto &p : this->emojiFirstByte_) {
        std::stable_sort(p.begin(), p.end(), [](const auto &lhs, const auto &rhs) {
            return lhs->value.length() > rhs->value.length();
        });
    }

    auto &p = this->shortCodes;
    std::stable_sort(p.begin(), p.end(),
                     [](const auto &lhs, const auto &rhs) { return lhs < rhs; });
}

void Emojis::loadEmojiSet()
{
    auto app = getApp();

    app->settings->emojiSet.connect([=](const auto &emojiSet, auto) {
        Log("Using emoji set {}", emojiSet);
        this->emojis.each([=](const auto &name, std::shared_ptr<EmojiData> &emoji) {
            QString emojiSetToUse = emojiSet;
            // clang-format off
            static std::map<QString, QString> emojiSets = {
                {"EmojiOne 2", "https://cdnjs.cloudflare.com/ajax/libs/emojione/2.2.6/assets/png/"},
                // {"EmojiOne 3", "https://cdn.jsdelivr.net/npm/emoji-datasource-emojione@4.0.4/img/emojione/64/"},
                // {"Twitter", "https://cdn.jsdelivr.net/npm/emoji-datasource-twitter@4.0.4/img/twitter/64/"},
                // {"Facebook", "https://cdn.jsdelivr.net/npm/emoji-datasource-facebook@4.0.4/img/facebook/64/"},
                // {"Apple", "https://cdn.jsdelivr.net/npm/emoji-datasource-apple@4.0.4/img/apple/64/"},
                // {"Google", "https://cdn.jsdelivr.net/npm/emoji-datasource-google@4.0.4/img/google/64/"},
                // {"Messenger", "https://cdn.jsdelivr.net/npm/emoji-datasource-messenger@4.0.4/img/messenger/64/"},

                // {"EmojiOne 2", "https://cdnjs.cloudflare.com/ajax/libs/emojione/2.2.6/assets/png/"},
                // {"EmojiOne 3", "https://braize.pajlada.com/emoji/img/emojione/64/"},
                // {"Twitter", "https://braize.pajlada.com/emoji/img/twitter/64/"},
                // {"Facebook", "https://braize.pajlada.com/emoji/img/facebook/64/"},
                // {"Apple", "https://braize.pajlada.com/emoji/img/apple/64/"},
                // {"Google", "https://braize.pajlada.com/emoji/img/google/64/"},
                // {"Messenger", "https://braize.pajlada.com/emoji/img/messenger/64/"},

                {"EmojiOne 3", "https://pajbot.com/static/emoji/img/emojione/64/"},
                {"Twitter", "https://pajbot.com/static/emoji/img/twitter/64/"},
                {"Facebook", "https://pajbot.com/static/emoji/img/facebook/64/"},
                {"Apple", "https://pajbot.com/static/emoji/img/apple/64/"},
                {"Google", "https://pajbot.com/static/emoji/img/google/64/"},
                {"Messenger", "https://pajbot.com/static/emoji/img/messenger/64/"},

//                {"EmojiOne 3", "https://cdn.fourtf.com/emoji/emojione/64/"},
//                {"Twitter", "https://cdn.fourtf.com/emoji/twitter/64/"},
//                {"Facebook", "https://cdn.fourtf.com/emoji/facebook/64/"},
//                {"Apple", "https://cdn.fourtf.com/emoji/apple/64/"},
//                {"Google", "https://cdn.fourtf.com/emoji/google/64/"},
//                {"Messenger", "https://cdn.fourtf.com/emoji/messenger/64/"},
            };
            // clang-format on

            if (emoji->capabilities.count(emojiSetToUse) == 0) {
                emojiSetToUse = "EmojiOne 3";
            }

            QString code = emoji->unifiedCode;
            if (emojiSetToUse == "EmojiOne 2") {
                if (!emoji->nonQualifiedCode.isEmpty()) {
                    code = emoji->nonQualifiedCode;
                }
            }
            code = code.toLower();
            QString urlPrefix = "https://cdnjs.cloudflare.com/ajax/libs/emojione/2.2.6/assets/png/";
            auto it = emojiSets.find(emojiSetToUse);
            if (it != emojiSets.end()) {
                urlPrefix = it->second;
            }
            QString url = urlPrefix + code + ".png";
            emoji->emote = std::make_shared<Emote>(
                Emote{EmoteName{emoji->value}, ImageSet{Image::fromUrl({url}, 0.35)},
                      Tooltip{":" + emoji->shortCodes[0] + ":<br/>Emoji"}, Url{}});
        });
    });
}

std::vector<boost::variant<EmotePtr, QString>> Emojis::parse(const QString &text)
{
    auto result = std::vector<boost::variant<EmotePtr, QString>>();
    int lastParsedEmojiEndIndex = 0;

    for (auto i = 0; i < text.length(); ++i) {
        const QChar character = text.at(i);

        if (character.isLowSurrogate()) {
            continue;
        }

        auto it = this->emojiFirstByte_.find(character);
        if (it == this->emojiFirstByte_.end()) {
            // No emoji starts with this character
            continue;
        }

        const auto &possibleEmojis = it.value();

        int remainingCharacters = text.length() - i - 1;

        std::shared_ptr<EmojiData> matchedEmoji;

        int matchedEmojiLength = 0;

        for (const std::shared_ptr<EmojiData> &emoji : possibleEmojis) {
            int emojiExtraCharacters = emoji->value.length() - 1;
            if (emojiExtraCharacters > remainingCharacters) {
                // It cannot be this emoji, there's not enough space for it
                continue;
            }

            bool match = true;

            for (int j = 1; j < emoji->value.length(); ++j) {
                if (text.at(i + j) != emoji->value.at(j)) {
                    match = false;

                    break;
                }
            }

            if (match) {
                matchedEmoji = emoji;
                matchedEmojiLength = emoji->value.length();

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
            result.emplace_back(text.mid(lastParsedEmojiEndIndex, charactersFromLastParsedEmoji));
        }

        // Push the emoji as a word to parsedWords
        result.emplace_back(matchedEmoji->emote);

        lastParsedEmojiEndIndex = currentParsedEmojiEndIndex;

        i += matchedEmojiLength - 1;
    }

    if (lastParsedEmojiEndIndex < text.length()) {
        // Add remaining characters
        result.emplace_back(text.mid(lastParsedEmojiEndIndex));
    }

    return result;
}

QString Emojis::replaceShortCodes(const QString &text)
{
    QString ret(text);
    auto it = this->findShortCodesRegex_.globalMatch(text);

    int32_t offset = 0;

    while (it.hasNext()) {
        auto match = it.next();

        auto capturedString = match.captured();

        QString matchString = capturedString.toLower().mid(1, capturedString.size() - 2);

        auto emojiIt = this->emojiShortCodeToEmoji_.constFind(matchString);

        if (emojiIt == this->emojiShortCodeToEmoji_.constEnd()) {
            continue;
        }

        auto emojiData = emojiIt.value();

        ret.replace(offset + match.capturedStart(), match.capturedLength(), emojiData->value);

        offset += emojiData->value.size() - match.capturedLength();
    }

    return ret;
}

}  // namespace chatterino
