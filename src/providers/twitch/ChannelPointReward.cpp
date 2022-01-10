#include "ChannelPointReward.hpp"
#include "common/QLogging.hpp"
#include "util/RapidjsonHelpers.hpp"

namespace chatterino {

QString parseRewardImage(const rapidjson::Value &obj, const char *key,
                         bool &result)
{
    QString url;
    if (!(result = rj::getSafe(obj, key, url)))
    {
        qCDebug(chatterinoTwitch)
            << "No url value found for key in reward image object:" << key;
        return "";
    }

    return url;
}

ChannelPointReward::ChannelPointReward(rapidjson::Value &redemption)
{
    rapidjson::Value user;
    if (!(this->hasParsedSuccessfully =
              rj::getSafeObject(redemption, "user", user)))
    {
        qCDebug(chatterinoTwitch) << "No user info found for redemption";
        return;
    }

    rapidjson::Value reward;
    if (!(this->hasParsedSuccessfully =
              rj::getSafeObject(redemption, "reward", reward)))
    {
        qCDebug(chatterinoTwitch) << "No reward info found for redemption";
        return;
    }

    if (!(this->hasParsedSuccessfully = rj::getSafe(reward, "id", this->id)))
    {
        qCDebug(chatterinoTwitch) << "No id found for reward";
        return;
    }

    if (!(this->hasParsedSuccessfully =
              rj::getSafe(reward, "channel_id", this->channelId)))
    {
        qCDebug(chatterinoTwitch) << "No channel_id found for reward";
        return;
    }

    if (!(this->hasParsedSuccessfully =
              rj::getSafe(reward, "title", this->title)))
    {
        qCDebug(chatterinoTwitch) << "No title found for reward";
        return;
    }

    if (!(this->hasParsedSuccessfully =
              rj::getSafe(reward, "cost", this->cost)))
    {
        qCDebug(chatterinoTwitch) << "No cost found for reward";
        return;
    }

    if (!(this->hasParsedSuccessfully = rj::getSafe(
              reward, "is_user_input_required", this->isUserInputRequired)))
    {
        qCDebug(chatterinoTwitch)
            << "No information if user input is required found for reward";
        return;
    }

    // We don't need to store user information for rewards with user input
    // because we will get the user info from a corresponding IRC message
    if (!this->isUserInputRequired)
    {
        this->parseUser(user);
    }

    rapidjson::Value obj;
    if (rj::getSafeObject(reward, "image", obj) && !obj.IsNull() &&
        obj.IsObject())
    {
        this->image = ImageSet{
            Image::fromUrl(
                {parseRewardImage(obj, "url_1x", this->hasParsedSuccessfully)},
                1),
            Image::fromUrl(
                {parseRewardImage(obj, "url_2x", this->hasParsedSuccessfully)},
                0.5),
            Image::fromUrl(
                {parseRewardImage(obj, "url_4x", this->hasParsedSuccessfully)},
                0.25),
        };
    }
    else
    {
        static const ImageSet defaultImage{
            Image::fromUrl({TWITCH_CHANNEL_POINT_REWARD_URL("1.png")}, 1),
            Image::fromUrl({TWITCH_CHANNEL_POINT_REWARD_URL("2.png")}, 0.5),
            Image::fromUrl({TWITCH_CHANNEL_POINT_REWARD_URL("4.png")}, 0.25)};
        this->image = defaultImage;
    }
}

void ChannelPointReward::parseUser(rapidjson::Value &user)
{
    if (!(this->hasParsedSuccessfully = rj::getSafe(user, "id", this->user.id)))
    {
        qCDebug(chatterinoTwitch) << "No id found for user in reward";
        return;
    }

    if (!(this->hasParsedSuccessfully =
              rj::getSafe(user, "login", this->user.login)))
    {
        qCDebug(chatterinoTwitch) << "No login name found for user in reward";
        return;
    }

    if (!(this->hasParsedSuccessfully =
              rj::getSafe(user, "display_name", this->user.displayName)))
    {
        qCDebug(chatterinoTwitch) << "No display name found for user in reward";
        return;
    }
}

}  // namespace chatterino
