#pragma once

namespace chatterino {

enum StreamerModeSetting { Disabled = 0, Enabled = 1, DetectObs = 2 };

const QStringList &broadcastingBinaries();
bool isInStreamerMode();

}  // namespace chatterino
