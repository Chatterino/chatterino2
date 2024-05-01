#pragma once
#include "common/Aliases.hpp"

#include <QPixmap>

namespace chatterino {

/**
 * Loads an image from url into a QPixmap. Allows for file:// protocol links. Uses cacheing.
 */
void loadPixmapFromUrlLazy(const Url &url,
                           std::function<void(QPixmap)> &&callback);
}  // namespace chatterino
