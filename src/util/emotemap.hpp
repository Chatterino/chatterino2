#pragma once

#include "messages/image.hpp"
#include "util/concurrentmap.hpp"

#include <cassert>

namespace chatterino {
namespace util {

struct EmoteData {
    EmoteData()
    {
    }

    EmoteData(messages::Image *_image)
        : image1x(_image)
    {
    }

    // Emotes must have a 1x image to be valid
    bool isValid() const
    {
        return this->image1x != nullptr;
    }

    messages::Image *image1x = nullptr;
    messages::Image *image2x = nullptr;
    messages::Image *image3x = nullptr;
};

typedef ConcurrentMap<QString, EmoteData> EmoteMap;

}  // namespace util
}  // namespace chatterino
