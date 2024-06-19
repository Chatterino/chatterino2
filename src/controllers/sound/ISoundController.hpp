#pragma once

#include <QUrl>

namespace chatterino {

enum class SoundBackend {
    Miniaudio,
    Null,
};

/**
 * @brief Handles sound loading & playback
 **/
class ISoundController
{
public:
    ISoundController() = default;
    virtual ~ISoundController() = default;
    ISoundController(const ISoundController &) = delete;
    ISoundController(ISoundController &&) = delete;
    ISoundController &operator=(const ISoundController &) = delete;
    ISoundController &operator=(ISoundController &&) = delete;

    // Play a sound from the given url
    // If the url points to something that isn't a local file, it will play
    // the default sound initialized in the initialize method
    //
    // This function should not block
    virtual void play(const QUrl &sound) = 0;
};

}  // namespace chatterino
