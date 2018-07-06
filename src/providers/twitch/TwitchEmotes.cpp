#include "providers/twitch/TwitchEmotes.hpp"

#include "common/UrlFetch.hpp"
#include "debug/Benchmark.hpp"
#include "debug/Log.hpp"
#include "messages/Image.hpp"
#include "util/RapidjsonHelpers.hpp"

#define TWITCH_EMOTE_TEMPLATE "https://static-cdn.jtvnw.net/emoticons/v1/{id}/{scale}"

namespace chatterino {

namespace {

QString getEmoteLink(const QString &id, const QString &emoteScale)
{
    QString value = TWITCH_EMOTE_TEMPLATE;

    value.detach();

    return value.replace("{id}", id).replace("{scale}", emoteScale);
}

QString cleanUpCode(const QString &dirtyEmoteCode)
{
    QString cleanCode = dirtyEmoteCode;
    // clang-format off
    static QMap<QString, QString> emoteNameReplacements{
        {"[oO](_|\\.)[oO]", "O_o"}, {"\\&gt\\;\\(", "&gt;("},   {"\\&lt\\;3", "&lt;3"},
        {"\\:-?(o|O)", ":O"},       {"\\:-?(p|P)", ":P"},       {"\\:-?[\\\\/]", ":/"},
        {"\\:-?[z|Z|\\|]", ":Z"},   {"\\:-?\\(", ":("},         {"\\:-?\\)", ":)"},
        {"\\:-?D", ":D"},           {"\\;-?(p|P)", ";P"},       {"\\;-?\\)", ";)"},
        {"R-?\\)", "R)"},           {"B-?\\)", "B)"},
    };
    // clang-format on

    auto it = emoteNameReplacements.find(dirtyEmoteCode);
    if (it != emoteNameReplacements.end()) {
        cleanCode = it.value();
    }

    cleanCode.replace("&lt;", "<");
    cleanCode.replace("&gt;", ">");

    return cleanCode;
}

void loadSetData(std::shared_ptr<TwitchEmotes::EmoteSet> emoteSet)
{
    Log("Load twitch emote set data for {}", emoteSet->key);
    NetworkRequest req("https://braize.pajlada.com/chatterino/twitchemotes/set/" + emoteSet->key +
                       "/");

    req.setRequestType(NetworkRequest::GetRequest);

    req.onError([](int errorCode) -> bool {
        Log("Emote sets on ERROR {}", errorCode);
        return true;
    });

    req.onSuccess([emoteSet](const rapidjson::Document &root) -> bool {
        Log("Emote sets on success");
        if (!root.IsObject()) {
            return false;
        }

        std::string emoteSetID;
        QString channelName;
        if (!rj::getSafe(root, "channel_name", channelName)) {
            return false;
        }

        emoteSet->channelName = channelName;

        return true;
    });

    req.execute();
}

}  // namespace

TwitchEmotes::TwitchEmotes()
{
    {
        EmoteSet emoteSet;
        emoteSet.key = "19194";
        emoteSet.text = "Twitch Prime Emotes";
        this->staticEmoteSets[emoteSet.key] = std::move(emoteSet);
    }

    {
        EmoteSet emoteSet;
        emoteSet.key = "0";
        emoteSet.text = "Twitch Global Emotes";
        this->staticEmoteSets[emoteSet.key] = std::move(emoteSet);
    }
}

// id is used for lookup
// emoteName is used for giving a name to the emote in case it doesn't exist
EmoteData TwitchEmotes::getEmoteById(const QString &id, const QString &emoteName)
{
    QString _emoteName = emoteName;
    _emoteName.replace("<", "&lt;");
    _emoteName.replace(">", "&gt;");

    // clang-format off
    static QMap<QString, QString> emoteNameReplacements{
        {"[oO](_|\\.)[oO]", "O_o"}, {"\\&gt\\;\\(", "&gt;("},   {"\\&lt\\;3", "&lt;3"},
        {"\\:-?(o|O)", ":O"},       {"\\:-?(p|P)", ":P"},       {"\\:-?[\\\\/]", ":/"},
        {"\\:-?[z|Z|\\|]", ":Z"},   {"\\:-?\\(", ":("},         {"\\:-?\\)", ":)"},
        {"\\:-?D", ":D"},           {"\\;-?(p|P)", ";P"},       {"\\;-?\\)", ";)"},
        {"R-?\\)", "R)"},           {"B-?\\)", "B)"},
    };
    // clang-format on

    auto it = emoteNameReplacements.find(_emoteName);
    if (it != emoteNameReplacements.end()) {
        _emoteName = it.value();
    }

    return twitchEmoteFromCache_.getOrAdd(id, [&emoteName, &_emoteName, &id] {
        EmoteData newEmoteData;
        auto cleanCode = cleanUpCode(emoteName);
        newEmoteData.image1x =
            new Image(getEmoteLink(id, "1.0"), 1, emoteName, _emoteName + "<br/>Twitch Emote");
        newEmoteData.image1x->setCopyString(cleanCode);

        newEmoteData.image2x =
            new Image(getEmoteLink(id, "2.0"), .5, emoteName, _emoteName + "<br/>Twitch Emote");
        newEmoteData.image2x->setCopyString(cleanCode);

        newEmoteData.image3x =
            new Image(getEmoteLink(id, "3.0"), .25, emoteName, _emoteName + "<br/>Twitch Emote");

        newEmoteData.image3x->setCopyString(cleanCode);

        return newEmoteData;
    });
}

void TwitchEmotes::refresh(const std::shared_ptr<TwitchAccount> &user)
{
    const auto &roomID = user->getUserId();
    TwitchAccountEmoteData &emoteData = this->emotes[roomID];

    if (emoteData.filled) {
        Log("Emotes are already loaded for room id {}", roomID);
        return;
    }

    auto loadEmotes = [=, &emoteData](const rapidjson::Document &root) {
        emoteData.emoteSets.clear();
        emoteData.emoteCodes.clear();

        auto emoticonSets = root.FindMember("emoticon_sets");
        if (emoticonSets == root.MemberEnd() || !emoticonSets->value.IsObject()) {
            Log("No emoticon_sets in load emotes response");
            return;
        }

        for (const auto &emoteSetJSON : emoticonSets->value.GetObject()) {
            auto emoteSet = std::make_shared<EmoteSet>();

            emoteSet->key = emoteSetJSON.name.GetString();

            loadSetData(emoteSet);

            for (const rapidjson::Value &emoteJSON : emoteSetJSON.value.GetArray()) {
                if (!emoteJSON.IsObject()) {
                    Log("Emote value was invalid");
                    return;
                }

                QString id, code;

                uint64_t idNumber;

                if (!rj::getSafe(emoteJSON, "id", idNumber)) {
                    Log("No ID key found in Emote value");
                    return;
                }

                if (!rj::getSafe(emoteJSON, "code", code)) {
                    Log("No code key found in Emote value");
                    return;
                }

                id = QString::number(idNumber);

                auto cleanCode = cleanUpCode(code);
                emoteSet->emotes.emplace_back(id, cleanCode);
                emoteData.emoteCodes.push_back(cleanCode);

                EmoteData emote = this->getEmoteById(id, code);
                emoteData.emotes.insert(code, emote);
            }

            emoteData.emoteSets.emplace_back(emoteSet);
        }

        emoteData.filled = true;
    };

    user->loadEmotes(loadEmotes);
}

void TwitchEmotes::loadSetData(std::shared_ptr<TwitchEmotes::EmoteSet> emoteSet)
{
    if (!emoteSet) {
        Log("null emote set sent");
        return;
    }

    auto staticSetIt = this->staticEmoteSets.find(emoteSet->key);
    if (staticSetIt != this->staticEmoteSets.end()) {
        const auto &staticSet = staticSetIt->second;
        emoteSet->channelName = staticSet.channelName;
        emoteSet->text = staticSet.text;
        return;
    }

    Log("Load twitch emote set data for {}..", emoteSet->key);
    NetworkRequest req("https://braize.pajlada.com/chatterino/twitchemotes/set/" + emoteSet->key +
                       "/");

    req.setRequestType(NetworkRequest::GetRequest);

    req.onError([](int errorCode) -> bool {
        Log("Emote sets on ERROR {}", errorCode);
        return true;
    });

    req.onSuccess([emoteSet](const rapidjson::Document &root) -> bool {
        if (!root.IsObject()) {
            return false;
        }

        std::string emoteSetID;
        QString channelName;
        QString type;
        if (!rj::getSafe(root, "channel_name", channelName)) {
            return false;
        }

        if (!rj::getSafe(root, "type", type)) {
            return false;
        }

        Log("Loaded twitch emote set data for {}!", emoteSet->key);

        if (type == "sub") {
            emoteSet->text = QString("Twitch Subscriber Emote (%1)").arg(channelName);
        } else {
            emoteSet->text = QString("Twitch Account Emote (%1)").arg(channelName);
        }

        emoteSet->channelName = channelName;

        return true;
    });

    req.execute();
}

}  // namespace chatterino
