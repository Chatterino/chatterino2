#include "Emotemap.hpp"

#include "Application.hpp"
#include "singletons/SettingsManager.hpp"

namespace chatterino {
namespace util {

EmoteData::EmoteData(messages::Image *_image)
    : image1x(_image)
{
}

// Emotes must have a 1x image to be valid
bool EmoteData::isValid() const
{
    return this->image1x != nullptr;
}

messages::Image *EmoteData::getImage(float scale) const
{
    int quality = getApp()->settings->preferredEmoteQuality;

    if (quality == 0) {
        scale *= getApp()->settings->emoteScale.getValue();
        quality = [&] {
            if (scale <= 1)
                return 1;
            if (scale <= 2)
                return 2;
            return 3;
        }();
    }

    messages::Image *_image;
    if (quality == 3 && this->image3x != nullptr) {
        _image = this->image3x;
    } else if (quality >= 2 && this->image2x != nullptr) {
        _image = this->image2x;
    } else {
        _image = this->image1x;
    }

    return _image;
}

}  // namespace util
}  // namespace chatterino
