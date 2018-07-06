#pragma once

#include "messages/Image.hpp"
#include "util/ConcurrentMap.hpp"

namespace chatterino {

struct EmoteData {
    EmoteData() = default;

    EmoteData(Image *_image);

    // Emotes must have a 1x image to be valid
    bool isValid() const;
    Image *getImage(float scale) const;

    // Link to the emote page i.e. https://www.frankerfacez.com/emoticon/144722-pajaCringe
    QString pageLink;

    Image *image1x = nullptr;
    Image *image2x = nullptr;
    Image *image3x = nullptr;
};

using EmoteMap = ConcurrentMap<QString, EmoteData>;

}  // namespace chatterino
