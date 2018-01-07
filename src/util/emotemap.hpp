#pragma once

#include "messages/lazyloadedimage.hpp"
#include "util/concurrentmap.hpp"

#include <cassert>

namespace chatterino {
namespace util {

struct EmoteData {
    EmoteData()
    {
    }

    EmoteData(messages::LazyLoadedImage *_image)
        : image1x(_image)
    {
    }

    messages::LazyLoadedImage *getImageForSize(unsigned emoteSize) const
    {
        messages::LazyLoadedImage *ret = nullptr;

        switch (emoteSize) {
            case 0:
                ret = this->image1x;
                break;
            case 1:
                ret = this->image2x;
                break;
            case 2:
                ret = this->image3x;
                break;

            default:
                ret = this->image1x;
                break;
        }

        if (ret == nullptr) {
            ret = this->image1x;
        }

        assert(ret != nullptr);

        return ret;
    }

    // Emotes must have a 1x image to be valid
    bool isValid() const
    {
        return this->image1x != nullptr;
    }

    messages::LazyLoadedImage *image1x = nullptr;
    messages::LazyLoadedImage *image2x = nullptr;
    messages::LazyLoadedImage *image3x = nullptr;
};

typedef ConcurrentMap<QString, EmoteData> EmoteMap;

}  // namespace util
}  // namespace chatterino
