#include "ChannelPointReward.hpp"
#include "util/RapidjsonHelpers.hpp"

namespace chatterino {

QString parseImage(const rapidjson::Value &obj, const char *key)
{
    QString url;
    assert(rj::getSafe(obj, key, url));

    return url;
}

ChannelPointReward::ChannelPointReward(rapidjson::Value &reward)
{
    assert(rj::getSafe(reward, "id", this->id));
    assert(rj::getSafe(reward, "channel_id", this->channelId));
    assert(rj::getSafe(reward, "title", this->title));
    assert(rj::getSafe(reward, "cost", this->cost));

    rapidjson::Value obj;
    if (rj::getSafeObject(reward, "image", obj) && !obj.IsNull() &&
        obj.IsObject())
    {
        this->image = ImageSet{
            Image::fromUrl({parseImage(obj, "url_1x")}, 1),
            Image::fromUrl({parseImage(obj, "url_2x")}, 0.5),
            Image::fromUrl({parseImage(obj, "url_4x")}, 0.25),
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
