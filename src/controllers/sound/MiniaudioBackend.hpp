// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/sound/ISoundController.hpp"
#include "util/OnceFlag.hpp"
#include "util/ThreadGuard.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <QByteArray>
#include <QString>
#include <QUrl>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <thread>

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
    // NUM_SOUNDS specifies how many simultaneous default ping sounds & decoders to create
    static constexpr const auto NUM_SOUNDS = 4;

    enum class State : std::uint8_t {
        Uninitialized,
        Initialized,
        Failed,
        Stopping,
    };

    std::atomic<State> state{State::Uninitialized};

public:
    explicit MiniaudioBackend(bool keepEngineAlive_);
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

    struct BuiltInSound {
        QByteArray data;
        // Stores N decoders for simultaneous default ping playback.
        // We can't use the engine API for this as this requires direct access to a custom data_source
        std::array<std::unique_ptr<ma_decoder>, NUM_SOUNDS> decoders;
        // Stores N sounds for simultaneous default ping playback
        // We can't use the engine API for this as this requires direct access to a custom data_source
        std::array<std::unique_ptr<ma_sound>, NUM_SOUNDS> sounds;
    };

    std::map<QUrl, BuiltInSound> defaultPingSounds;

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

    /// This setting controls whether the miniaudio sound engine should be kept alive at all times
    bool keepEngineAlive;

    friend class Application;
};

}  // namespace chatterino
