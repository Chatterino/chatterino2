#pragma once

#include "messages/Image.hpp"
#include "util/ConcurrentMap.hpp"

namespace chatterino {

struct EmoteData {
    EmoteData() = default;

    EmoteData(messages::Image *_image);

    // Emotes must have a 1x image to be valid
    bool isValid() const;
    messages::Image *getImage(float scale) const;

    messages::Image *image1x = nullptr;
    messages::Image *image2x = nullptr;
    messages::Image *image3x = nullptr;

    // Link to the emote page i.e. https://www.frankerfacez.com/emoticon/144722-pajaCringe
    QString pageLink;
};

using EmoteMap = ConcurrentMap<QString, EmoteData>;

}  // namespace chatterino
