#pragma once

#include "util/emotemap.hpp"

#include <QString>

namespace chatterino {

struct EmojiData {
    // actual byte-representation of the emoji (i.e. \154075\156150 which is :male:)
    QString value;

    // what's used in the emoji-one url
    QString code;

    // i.e. thinking
    QString shortCode;

    util::EmoteData emoteData;
};

namespace util {

using EmojiMap = ConcurrentMap<QString, EmojiData>;

}  // namespace util

}  // namespace chatterino
