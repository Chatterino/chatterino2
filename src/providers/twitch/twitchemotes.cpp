#include "providers/twitch/twitchemotes.hpp"

#include "debug/log.hpp"
#include "messages/image.hpp"
#include "util/urlfetch.hpp"

#define TWITCH_EMOTE_TEMPLATE "https://static-cdn.jtvnw.net/emoticons/v1/{id}/{scale}"

namespace chatterino {
namespace providers {
namespace twitch {

namespace {

QString getEmoteLink(const QString &id, const QString &emoteScale)
{
    QString value = TWITCH_EMOTE_TEMPLATE;

    value.detach();

    return value.replace("{id}", id).replace("{scale}", emoteScale);
}

}  // namespace

// id is used for lookup
// emoteName is used for giving a name to the emote in case it doesn't exist
util::EmoteData TwitchEmotes::getEmoteById(const QString &id, const QString &emoteName)
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

    return _twitchEmoteFromCache.getOrAdd(id, [&emoteName, &_emoteName, &id] {
        util::EmoteData newEmoteData;
        newEmoteData.image1x = new messages::Image(getEmoteLink(id, "1.0"), 1, emoteName,
                                                   _emoteName + "<br/>Twitch Emote 1x");
        newEmoteData.image2x = new messages::Image(getEmoteLink(id, "2.0"), .5, emoteName,
                                                   _emoteName + "<br/>Twitch Emote 2x");
        newEmoteData.image3x = new messages::Image(getEmoteLink(id, "3.0"), .25, emoteName,
                                                   _emoteName + "<br/>Twitch Emote 3x");

        return newEmoteData;
    });
}

void TwitchEmotes::refresh(const std::shared_ptr<TwitchAccount> &user)
{
    debug::Log("Loading Twitch emotes for user {}", user->getUserName());

    const auto &roomID = user->getUserId();
    const auto &clientID = user->getOAuthClient();
    const auto &oauthToken = user->getOAuthToken();

    if (clientID.isEmpty() || oauthToken.isEmpty()) {
        debug::Log("Missing Client ID or OAuth token");
        return;
    }

    TwitchAccountEmoteData &emoteData = this->emotes[roomID];

    if (emoteData.filled) {
        debug::Log("Emotes are already loaded for room id {}", roomID);
        return;
    }

    QString url("https://api.twitch.tv/kraken/users/" + roomID + "/emotes");

    auto loadEmotes = [=, &emoteData](const QJsonObject &root) {
        emoteData.emoteSets.clear();
        emoteData.emoteCodes.clear();

        auto emoticonSets = root.value("emoticon_sets").toObject();
        for (QJsonObject::iterator it = emoticonSets.begin(); it != emoticonSets.end(); ++it) {
            EmoteSet emoteSet;

            emoteSet.key = it.key();

            for (QJsonValue emoteValue : it.value().toArray()) {
                QJsonObject emoticon = emoteValue.toObject();
                QString id = QString::number(emoticon["id"].toInt());
                QString code = emoticon["code"].toString();
                emoteSet.emotes.emplace_back(id, code);
                emoteData.emoteCodes.push_back(code);

                util::EmoteData emote = this->getEmoteById(id, code);
                emoteData.emotes.insert(code, emote);
            }

            emoteData.emoteSets.emplace_back(std::move(emoteSet));
        }

        emoteData.filled = true;
    };

    util::twitch::getAuthorized(url, clientID, oauthToken, QThread::currentThread(), loadEmotes);
}

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
