#include "providers/emoji/Emojis.hpp"

#include "common/QLogging.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "singletons/Settings.hpp"
#include "util/QMagicEnum.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <boost/variant.hpp>
#include <QFile>
#include <rapidjson/error/en.h>
#include <rapidjson/error/error.h>
#include <rapidjson/rapidjson.h>

#include <map>
#include <memory>

namespace {

using namespace chatterino;

const std::map<QString, QString> TONE_NAMES{
    {"1F3FB", "tone1"}, {"1F3FC", "tone2"}, {"1F3FD", "tone3"},
    {"1F3FE", "tone4"}, {"1F3FF", "tone5"},
};

void parseEmoji(const std::shared_ptr<EmojiData> &emojiData,
                const rapidjson::Value &unparsedEmoji,
                const QString &shortCode = {})
{
    std::vector<char32_t> unicodeBytes{};

    struct {
        bool apple;
        bool google;
        bool twitter;
        bool facebook;
    } capabilities{};

    if (!shortCode.isEmpty())
    {
        emojiData->shortCodes.push_back(shortCode);
    }
    else
    {
        // Load short codes from the suggested short_names
        const auto &shortNames = unparsedEmoji["short_names"];
        for (const auto &shortName : shortNames.GetArray())
        {
            emojiData->shortCodes.emplace_back(shortName.GetString());
        }
    }

    rj::getSafe(unparsedEmoji, "non_qualified", emojiData->nonQualifiedCode);
    rj::getSafe(unparsedEmoji, "unified", emojiData->unifiedCode);
    assert(!emojiData->unifiedCode.isEmpty());

    rj::getSafe(unparsedEmoji, "has_img_apple", capabilities.apple);
    rj::getSafe(unparsedEmoji, "has_img_google", capabilities.google);
    rj::getSafe(unparsedEmoji, "has_img_twitter", capabilities.twitter);
    rj::getSafe(unparsedEmoji, "has_img_facebook", capabilities.facebook);

    if (capabilities.apple)
    {
        emojiData->capabilities.set(EmojiData::Capability::Apple);
    }
    if (capabilities.google)
    {
        emojiData->capabilities.set(EmojiData::Capability::Google);
    }
    if (capabilities.twitter)
    {
        emojiData->capabilities.set(EmojiData::Capability::Twitter);
    }
    if (capabilities.facebook)
    {
        emojiData->capabilities.set(EmojiData::Capability::Facebook);
    }

    QStringList unicodeCharacters = emojiData->unifiedCode.toLower().split('-');

    for (const QString &unicodeCharacter : unicodeCharacters)
    {
        bool ok{false};
        unicodeBytes.push_back(unicodeCharacter.toUInt(&ok, 16));
        if (!ok)
        {
            qCWarning(chatterinoEmoji)
                << "Failed to parse emoji" << emojiData->shortCodes;
            return;
        }
    }

    // We can safely do a narrowing static cast since unicodeBytes will never be a large number
    emojiData->value =
        QString::fromUcs4(unicodeBytes.data(),
                          static_cast<QString::size_type>(unicodeBytes.size()));

    if (!emojiData->nonQualifiedCode.isEmpty())
    {
        QStringList nonQualifiedCharacters =
            emojiData->nonQualifiedCode.toLower().split('-');
        std::vector<char32_t> nonQualifiedBytes{};
        for (const QString &unicodeCharacter : nonQualifiedCharacters)
        {
            bool ok{false};
            nonQualifiedBytes.push_back(
                QString(unicodeCharacter).toUInt(&ok, 16));
            if (!ok)
            {
                qCWarning(chatterinoEmoji)
                    << "Failed to parse emoji nonQualified"
                    << emojiData->shortCodes;
                return;
            }
        }

        // We can safely do a narrowing static cast since unicodeBytes will never be a large number
        emojiData->nonQualified = QString::fromUcs4(
            nonQualifiedBytes.data(),
            static_cast<QString::size_type>(nonQualifiedBytes.size()));
    }
}

// getToneNames takes a tones and returns their names in the same order
// The format of the tones is: "1F3FB-1F3FB" or "1F3FB"
// The output of the tone names is: "tone1-tone1" or "tone1"
QString getToneNames(const QString &tones)
{
    auto toneParts = tones.split('-');
    QStringList toneNameResults;
    for (const auto &tonePart : toneParts)
    {
        auto toneNameIt = TONE_NAMES.find(tonePart);
        if (toneNameIt == TONE_NAMES.end())
        {
            qDebug() << "Tone with key" << tonePart
                     << "does not exist in tone names map";
            continue;
        }

        toneNameResults.append(toneNameIt->second);
    }

    assert(!toneNameResults.isEmpty());

    return toneNameResults.join('-');
}

}  // namespace

namespace chatterino {

void Emojis::load()
{
    if (this->loaded_)
    {
        return;
    }
    this->loaded_ = true;

    this->loadEmojis();

    this->sortEmojis();

    this->loadEmojiSet();
}

void Emojis::loadEmojis()
{
    // Current version: https://github.com/iamcal/emoji-data/blob/v15.1.1/emoji.json (Emoji version 15.1 (2023))
    QFile file(":/emoji.json");
    file.open(QFile::ReadOnly);
    QTextStream s1(&file);
    QString data = s1.readAll();
    rapidjson::Document root;
    rapidjson::ParseResult result = root.Parse(data.toUtf8(), data.length());

    if (result.Code() != rapidjson::kParseErrorNone)
    {
        qCWarning(chatterinoEmoji)
            << "JSON parse error:" << rapidjson::GetParseError_En(result.Code())
            << "(" << result.Offset() << ")";
        return;
    }

    for (const auto &unparsedEmoji : root.GetArray())
    {
        auto emojiData = std::make_shared<EmojiData>();
        parseEmoji(emojiData, unparsedEmoji);

        for (const auto &shortCode : emojiData->shortCodes)
        {
            this->emojiShortCodeToEmoji_.insert(shortCode, emojiData);
            this->shortCodes.emplace_back(shortCode);
        }

        this->emojiFirstByte_[emojiData->value.at(0)].append(emojiData);

        this->emojis.push_back(emojiData);

        if (unparsedEmoji.HasMember("skin_variations"))
        {
            for (const auto &skinVariation :
                 unparsedEmoji["skin_variations"].GetObject())
            {
                auto toneName = getToneNames(skinVariation.name.GetString());
                const auto &variation = skinVariation.value;

                auto variationEmojiData = std::make_shared<EmojiData>();

                parseEmoji(variationEmojiData, variation,
                           emojiData->shortCodes[0] + "_" + toneName);

                this->emojiShortCodeToEmoji_.insert(
                    variationEmojiData->shortCodes[0], variationEmojiData);
                this->shortCodes.push_back(variationEmojiData->shortCodes[0]);

                this->emojiFirstByte_[variationEmojiData->value.at(0)].append(
                    variationEmojiData);

                this->emojis.push_back(variationEmojiData);
            }
        }
    }
}

void Emojis::sortEmojis()
{
    for (auto &p : this->emojiFirstByte_)
    {
        std::stable_sort(p.begin(), p.end(),
                         [](const auto &lhs, const auto &rhs) {
                             return lhs->value.length() > rhs->value.length();
                         });
    }

    auto &p = this->shortCodes;
    std::stable_sort(p.begin(), p.end(), [](const auto &lhs, const auto &rhs) {
        return lhs < rhs;
    });
}

void Emojis::loadEmojiSet()
{
    getSettings()->emojiSet.connect([this](const auto &emojiSet) {
        EmojiData::Capability setCapability =
            qmagicenum::enumCast<EmojiData::Capability>(emojiSet).value_or(
                EmojiData::Capability::Google);

        for (const auto &emoji : this->emojis)
        {
            QString emojiSetToUse = emojiSet;
            // clang-format off
            static std::map<QString, QString> emojiSets = {
                // JSDELIVR
                // {"Twitter", "https://cdn.jsdelivr.net/npm/emoji-datasource-twitter@4.0.4/img/twitter/64/"},
                // {"Facebook", "https://cdn.jsdelivr.net/npm/emoji-datasource-facebook@4.0.4/img/facebook/64/"},
                // {"Apple", "https://cdn.jsdelivr.net/npm/emoji-datasource-apple@5.0.1/img/apple/64/"},
                // {"Google", "https://cdn.jsdelivr.net/npm/emoji-datasource-google@4.0.4/img/google/64/"},
                // {"Messenger", "https://cdn.jsdelivr.net/npm/emoji-datasource-messenger@4.0.4/img/messenger/64/"},

                // OBRODAI
                {"Twitter", "https://pajbot.com/static/emoji-v2/img/twitter/64/"},
                {"Facebook", "https://pajbot.com/static/emoji-v2/img/facebook/64/"},
                {"Apple", "https://pajbot.com/static/emoji-v2/img/apple/64/"},
                {"Google", "https://pajbot.com/static/emoji-v2/img/google/64/"},

                // Cloudflare+B2 bucket
                // {"Twitter", "https://chatterino2-emoji-cdn.pajlada.se/file/c2-emojis/emojis-v1/twitter/64/"},
                // {"Facebook", "https://chatterino2-emoji-cdn.pajlada.se/file/c2-emojis/emojis-v1/facebook/64/"},
                // {"Apple", "https://chatterino2-emoji-cdn.pajlada.se/file/c2-emojis/emojis-v1/apple/64/"},
                // {"Google", "https://chatterino2-emoji-cdn.pajlada.se/file/c2-emojis/emojis-v1/google/64/"},
            };
            // clang-format on

            // As of emoji-data v15.1.1, google is the only source missing no images.
            if (!emoji->capabilities.has(setCapability))
            {
                emojiSetToUse = QStringLiteral("Google");
            }

            QString code = emoji->unifiedCode.toLower();
            QString urlPrefix =
                "https://pajbot.com/static/emoji-v2/img/google/64/";
            auto it = emojiSets.find(emojiSetToUse);
            if (it != emojiSets.end())
            {
                urlPrefix = it->second;
            }
            QString url = urlPrefix + code + ".png";
            emoji->emote = std::make_shared<Emote>(Emote{
                EmoteName{emoji->value},
                ImageSet{Image::fromUrl({url}, 0.35, {64, 64})},
                Tooltip{":" + emoji->shortCodes[0] + ":<br/>Emoji"}, Url{}});
        }
    });
}

std::vector<boost::variant<EmotePtr, QString>> Emojis::parse(
    const QString &text) const
{
    auto result = std::vector<boost::variant<EmotePtr, QString>>();
    QString::size_type lastParsedEmojiEndIndex = 0;

    for (qsizetype i = 0; i < text.length(); ++i)
    {
        const QChar character = text.at(i);

        if (character.isLowSurrogate())
        {
            continue;
        }

        auto it = this->emojiFirstByte_.find(character);
        if (it == this->emojiFirstByte_.end())
        {
            // No emoji starts with this character
            continue;
        }

        const auto &possibleEmojis = it.value();

        auto remainingCharacters = text.length() - i - 1;

        std::shared_ptr<EmojiData> matchedEmoji;

        QString::size_type matchedEmojiLength = 0;

        for (const std::shared_ptr<EmojiData> &emoji : possibleEmojis)
        {
            auto emojiNonQualifiedExtraCharacters =
                emoji->nonQualified.length() - 1;
            auto emojiExtraCharacters = emoji->value.length() - 1;
            if (remainingCharacters >= emojiExtraCharacters)
            {
                // look in emoji->value
                bool match = QStringView{emoji->value}.mid(1) ==
                             QStringView{text}.mid(i + 1, emojiExtraCharacters);

                if (match)
                {
                    matchedEmoji = emoji;
                    matchedEmojiLength = emoji->value.length();

                    break;
                }
            }
            if (!emoji->nonQualified.isNull() &&
                remainingCharacters >= emojiNonQualifiedExtraCharacters)
            {
                // This checking here relies on the fact that the nonQualified string
                // always starts with the same byte as value (the unified string)
                bool match = QStringView{emoji->nonQualified}.mid(1) ==
                             QStringView{text}.mid(
                                 i + 1, emojiNonQualifiedExtraCharacters);

                if (match)
                {
                    matchedEmoji = emoji;
                    matchedEmojiLength = emoji->nonQualified.length();

                    break;
                }
            }
        }

        if (matchedEmojiLength == 0)
        {
            continue;
        }

        auto currentParsedEmojiFirstIndex = i;
        auto currentParsedEmojiEndIndex = i + (matchedEmojiLength);

        auto charactersFromLastParsedEmoji =
            currentParsedEmojiFirstIndex - lastParsedEmojiEndIndex;

        if (charactersFromLastParsedEmoji > 0)
        {
            // Add characters inbetween emojis
            result.emplace_back(text.mid(lastParsedEmojiEndIndex,
                                         charactersFromLastParsedEmoji));
        }

        // Push the emoji as a word to parsedWords
        result.emplace_back(matchedEmoji->emote);

        lastParsedEmojiEndIndex = currentParsedEmojiEndIndex;

        i += matchedEmojiLength - 1;
    }

    if (lastParsedEmojiEndIndex < text.length())
    {
        // Add remaining characters
        result.emplace_back(text.mid(lastParsedEmojiEndIndex));
    }

    return result;
}

QString Emojis::replaceShortCodes(const QString &text) const
{
    QString ret(text);
    auto it = this->findShortCodesRegex_.globalMatch(text);

    qsizetype offset = 0;

    while (it.hasNext())
    {
        auto match = it.next();

        auto capturedString = match.captured();

        QString matchString =
            capturedString.toLower().mid(1, capturedString.size() - 2);

        auto emojiIt = this->emojiShortCodeToEmoji_.constFind(matchString);

        if (emojiIt == this->emojiShortCodeToEmoji_.constEnd())
        {
            continue;
        }

        const auto &emojiData = emojiIt.value();

        ret.replace(offset + match.capturedStart(), match.capturedLength(),
                    emojiData->value);

        offset += emojiData->value.size() - match.capturedLength();
    }

    return ret;
}

const std::vector<EmojiPtr> &Emojis::getEmojis() const
{
    return this->emojis;
}

const std::vector<QString> &Emojis::getShortCodes() const
{
    return this->shortCodes;
}

}  // namespace chatterino
