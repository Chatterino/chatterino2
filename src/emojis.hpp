#pragma once

#include "messages/lazyloadedimage.hpp"
#include "util/concurrentmap.hpp"

#include <QObject>
#include <QString>

#include <unordered_map>

namespace chatterino {

struct EmojiData {
    QString value;

    // what's used in the emoji-one url
    QString code;

    // i.e. thinking
    QString shortCode;
};

}  // namespace chatterino
