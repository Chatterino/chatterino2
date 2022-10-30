#pragma once

#include <QStringList>

namespace chatterino {

enum StreamerModeSetting {
    Disabled = 0,
    Enabled = 1,
    DetectStreamingSoftware = 2,
};

const QStringList &broadcastingBinaries();
bool isInStreamerMode();

}  // namespace chatterino
