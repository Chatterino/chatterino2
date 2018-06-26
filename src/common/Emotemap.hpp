#pragma once

#include "messages/Image.hpp"
#include "util/ConcurrentMap.hpp"

namespace chatterino {

struct EmoteData {
    EmoteData() = default;

    EmoteData(chatterino::Image *_image);

    // Emotes must have a 1x image to be valid
    bool isValid() const;
    chatterino::Image *getImage(float scale) const;

    chatterino::Image *image1x = nullptr;
    chatterino::Image *image2x = nullptr;
    chatterino::Image *image3x = nullptr;

    // Link to the emote page i.e. https://www.frankerfacez.com/emoticon/144722-pajaCringe
    QString pageLink;
};

using EmoteMap = ConcurrentMap<QString, EmoteData>;

}  // namespace chatterino
