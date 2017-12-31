#pragma once

#include "messages/lazyloadedimage.hpp"
#include "util/concurrentmap.hpp"

namespace chatterino {
namespace util {

struct EmoteData {
    EmoteData()
    {
    }

    EmoteData(messages::LazyLoadedImage *_image)
        : image(_image)
    {
    }

    messages::LazyLoadedImage *image = nullptr;
};

typedef ConcurrentMap<QString, EmoteData> EmoteMap;
}
}
