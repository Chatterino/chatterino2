#pragma once

#include "controllers/sound/ISoundController.hpp"

namespace chatterino {

/**
 * @brief This sound backend does nothing
 **/
class NullBackend final : public ISoundController
{
public:
    NullBackend();
    ~NullBackend() override = default;
    NullBackend(const NullBackend &) = delete;
    NullBackend(NullBackend &&) = delete;
    NullBackend &operator=(const NullBackend &) = delete;
    NullBackend &operator=(NullBackend &&) = delete;

    // Play a sound from the given url
    // If the url points to something that isn't a local file, it will play
    // the default sound initialized in the initialize method
    void play(const QUrl &sound) final;
};

}  // namespace chatterino
