#pragma once

#include "common/Singleton.hpp"
#include "util/ThreadGuard.hpp"

#include <QByteArray>
#include <QString>
#include <QUrl>

#include <memory>
#include <vector>

struct ma_engine;
struct ma_device;
struct ma_resource_manager;
struct ma_context;
struct ma_sound;
struct ma_decoder;

namespace chatterino {

class Settings;
class Paths;

/**
 * @brief Handles sound loading & playback
 **/
class SoundController : public Singleton
{
    SoundController();

    void initialize(Settings &settings, Paths &paths) override;

public:
    ~SoundController() override;

    // Play a sound from the given url
    // If the url points to something that isn't a local file, it will play
    // the default sound initialized in the initialize method
    void play(const QUrl &sound);

private:
    // Used for selecting & initializing an appropriate sound backend
    std::unique_ptr<ma_context> context;
    // Used for storing & reusing sounds to be played
    std::unique_ptr<ma_resource_manager> resourceManager;
    // The sound device we're playing sound into
    std::unique_ptr<ma_device> device;
    // The engine is a high-level API for playing sounds from paths in a simple & efficient-enough manner
    std::unique_ptr<ma_engine> engine;

    // Stores the data of our default ping sounds
    QByteArray defaultPingData;
    // Stores N decoders for simultaneous default ping playback.
    // We can't use the engine API for this as this requires direct access to a custom data_source
    std::vector<std::unique_ptr<ma_decoder>> defaultPingDecoders;
    // Stores N sounds for simultaneous default ping playback
    // We can't use the engine API for this as this requires direct access to a custom data_source
    std::vector<std::unique_ptr<ma_sound>> defaultPingSounds;

    // Thread guard for the play method
    // Ensures play is only ever called from the same thread
    ThreadGuard tgPlay;

    bool initialized{false};

    friend class Application;
};

}  // namespace chatterino
