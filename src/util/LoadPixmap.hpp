#pragma once
#include "common/Aliases.hpp"

#include <QPixmap>

namespace chatterino {

/**
 * Loads an image from url into a QPixmap. Allows for file:// protocol links. Uses cacheing.
 *
 * @param callback The callback you will get the pixmap by. It will be invoked concurrently with no guarantees on which thread.
 */
void loadPixmapFromUrl(const Url &url, std::function<void(QPixmap)> &&callback);

}  // namespace chatterino
