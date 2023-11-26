#include "controllers/sound/NullBackend.hpp"

#include "common/QLogging.hpp"

namespace chatterino {

NullBackend::NullBackend()
{
    qCInfo(chatterinoSound) << "Initializing null sound backend";
}

void NullBackend::play(const QUrl &sound)
{
    // Do nothing
    qCDebug(chatterinoSound) << "null backend asked to play" << sound;
}

}  // namespace chatterino
