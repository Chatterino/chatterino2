#include "ChannelPointReward.hpp"

#include "common/QLogging.hpp"
#include "messages/Image.hpp"

namespace chatterino {

ChannelPointReward::ChannelPointReward(const QJsonObject &redemption)
{
    auto reward = redemption.value("reward").toObject();

    this->id = reward.value("id").toString();
    this->channelId = reward.value("channel_id").toString();
    this->title = reward.value("title").toString();
    this->cost = reward.value("cost").toInt();
    this->isUserInputRequired = reward.value("is_user_input_required").toBool();

    // We don't need to store user information for rewards with user input
    // because we will get the user info from a corresponding IRC message
    if (!this->isUserInputRequired)
    {
        auto user = redemption.value("user").toObject();

        this->user.id = user.value("id").toString();
        this->user.login = user.value("login").toString();
        this->user.displayName = user.value("display_name").toString();
    }

    auto imageValue = reward.value("image");
    // From Twitch docs
    // The size is only an estimation, the actual size might vary.
    constexpr QSize baseSize(28, 28);

    if (imageValue.isObject())
    {
        auto imageObject = imageValue.toObject();
        this->image = ImageSet{
            Image::fromUrl({imageObject.value("url_1x").toString()}, 1,
                           baseSize),
            Image::fromUrl({imageObject.value("url_2x").toString()}, 0.5,
                           baseSize * 2),
            Image::fromUrl({imageObject.value("url_4x").toString()}, 0.25,
                           baseSize * 4),
        };
    }
    else
    {
        static const ImageSet defaultImage{
            Image::fromUrl({TWITCH_CHANNEL_POINT_REWARD_URL("1.png")}, 1,
                           baseSize),
            Image::fromUrl({TWITCH_CHANNEL_POINT_REWARD_URL("2.png")}, 0.5,
                           baseSize * 2),
            Image::fromUrl({TWITCH_CHANNEL_POINT_REWARD_URL("4.png")}, 0.25,
                           baseSize * 4)};
        this->image = defaultImage;
    }
}

}  // namespace chatterino
