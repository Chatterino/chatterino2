#pragma once
#include "common/Aliases.hpp"

#include <QPixmap>

namespace chatterino {

/**
 * Loads an image from url into a QPixmap. Allows for file:// protocol links. Uses cacheing.
 *
 * @param onSuccess The callback you will get the pixmap by. It will be invoked concurrently with no guarantees on which thread.
 * @param onError The callback that will be called if the request fails or the image cannot be loaded.
 */
void loadPixmapFromUrl(
    const Url &url, std::function<void(QPixmap)> &&onSuccess,
    const std::optional<std::function<void()>> &onError = {});

}  // namespace chatterino
