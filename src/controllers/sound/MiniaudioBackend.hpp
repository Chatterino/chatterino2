#pragma once

#include "controllers/sound/ISoundController.hpp"
#include "util/OnceFlag.hpp"
#include "util/ThreadGuard.hpp"

#include <boost/asio.hpp>
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

/**
 * @brief Handles sound loading & playback
 **/
class MiniaudioBackend : public ISoundController
{
public:
    MiniaudioBackend();
    ~MiniaudioBackend() override;

    // Play a sound from the given url
    // If the url points to something that isn't a local file, it will play
    // the default sound initialized in the initialize method
    void play(const QUrl &sound) final;

private:
    // Used for selecting & initializing an appropriate sound backend
    std::unique_ptr<ma_context> context;
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

    std::chrono::system_clock::time_point lastSoundPlay;

    boost::asio::io_context ioContext{1};
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        workGuard;
    std::unique_ptr<std::thread> audioThread;
    OnceFlag stoppedFlag;
    boost::asio::steady_timer sleepTimer;

    bool initialized{false};

    friend class Application;
};

}  // namespace chatterino
