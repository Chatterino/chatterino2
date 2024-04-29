#pragma once
#include "common/Aliases.hpp"

#include <QPixmap>

namespace chatterino {
void loadPixmapFromUrlLazy(const Url &url,
                           std::function<void(QPixmap)> &&callback);
}  // namespace chatterino
