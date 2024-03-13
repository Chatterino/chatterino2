#include "providers/seventv/SeventvAPI.hpp"

#include "common/Literals.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"

namespace {

using namespace chatterino::literals;

const QString API_URL_USER = u"https://7tv.io/v3/users/twitch/%1"_s;
const QString API_URL_EMOTE_SET = u"https://7tv.io/v3/emote-sets/%1"_s;
const QString API_URL_PRESENCES = u"https://7tv.io/v3/users/%1/presences"_s;

}  // namespace

// NOLINTBEGIN(readability-convert-member-functions-to-static)
namespace chatterino {

void SeventvAPI::getUserByTwitchID(
    const QString &twitchID, SuccessCallback<const QJsonObject &> &&onSuccess,
    ErrorCallback &&onError)
{
    NetworkRequest(API_URL_USER.arg(twitchID), NetworkRequestType::Get)
        .timeout(20000)
        .onSuccess(
            [callback = std::move(onSuccess)](const NetworkResult &result) {
                auto json = result.parseJson();
                callback(json);
            })
        .onError([callback = std::move(onError)](const NetworkResult &result) {
            callback(result);
        })
        .execute();
}

void SeventvAPI::getEmoteSet(const QString &emoteSet,
                             SuccessCallback<const QJsonObject &> &&onSuccess,
                             ErrorCallback &&onError)
{
    NetworkRequest(API_URL_EMOTE_SET.arg(emoteSet), NetworkRequestType::Get)
        .timeout(25000)
        .onSuccess(
            [callback = std::move(onSuccess)](const NetworkResult &result) {
                auto json = result.parseJson();
                callback(json);
            })
        .onError([callback = std::move(onError)](const NetworkResult &result) {
            callback(result);
        })
        .execute();
}

void SeventvAPI::updatePresence(const QString &twitchChannelID,
                                const QString &seventvUserID,
                                SuccessCallback<> &&onSuccess,
                                ErrorCallback &&onError)
{
    QJsonObject payload{
        {u"kind"_s, 1},  // UserPresenceKindChannel
        {u"data"_s,
         QJsonObject{
             {u"id"_s, twitchChannelID},
             {u"platform"_s, u"TWITCH"_s},
         }},
    };

    NetworkRequest(API_URL_PRESENCES.arg(seventvUserID),
                   NetworkRequestType::Post)
        .json(payload)
        .timeout(10000)
        .onSuccess([callback = std::move(onSuccess)](const auto &) {
            callback();
        })
        .onError([callback = std::move(onError)](const NetworkResult &result) {
            callback(result);
        })
        .execute();
}

}  // namespace chatterino
// NOLINTEND(readability-convert-member-functions-to-static)
