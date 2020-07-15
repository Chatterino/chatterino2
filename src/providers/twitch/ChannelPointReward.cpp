#include "ChannelPointReward.hpp"
#include "util/RapidjsonHelpers.hpp"

namespace chatterino {

QString parseImage(const rapidjson::Value &obj, const char *key, bool &result)
{
    QString url;
    if (!(result = rj::getSafe(obj, key, url)))
    {
        qDebug() << "No url value found for key in reward image object:" << key;
        return "";
    }

    return url;
}

ChannelPointReward::ChannelPointReward(rapidjson::Value &reward)
{
    if (!(this->hasParsedSuccessfully = rj::getSafe(reward, "id", this->id)))
    {
        qDebug() << "No id found for reward";
        return;
    }

    if (!(this->hasParsedSuccessfully =
              rj::getSafe(reward, "channel_id", this->channelId)))
    {
        qDebug() << "No channel_id found for reward";
        return;
    }

    if (!(this->hasParsedSuccessfully =
              rj::getSafe(reward, "title", this->title)))
    {
        qDebug() << "No title found for reward";
        return;
    }

    if (!(this->hasParsedSuccessfully =
              rj::getSafe(reward, "cost", this->cost)))
    {
        qDebug() << "No cost found for reward";
        return;
    }

    rapidjson::Value obj;
    if (rj::getSafeObject(reward, "image", obj) && !obj.IsNull() &&
        obj.IsObject())
    {
        this->image = ImageSet{
            Image::fromUrl(
                {parseImage(obj, "url_1x", this->hasParsedSuccessfully)}, 1),
            Image::fromUrl(
                {parseImage(obj, "url_2x", this->hasParsedSuccessfully)}, 0.5),
            Image::fromUrl(
                {parseImage(obj, "url_4x", this->hasParsedSuccessfully)}, 0.25),
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

}  // namespace chatterino
