#pragma once
#include "common/Aliases.hpp"

namespace chatterino {
void loadPixmapFromUrlLazy(const Url &url,
                           std::function<void(QPixmap)> &&callback);
}  // namespace chatterino
