// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/sound/MiniaudioBackend.hpp"

#include "common/QLogging.hpp"
#include "controllers/highlights/Sounds.hpp"
#include "debug/Benchmark.hpp"
#include "util/QMagicEnum.hpp"
#include "util/RenameThread.hpp"

#include <boost/asio.hpp>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <QFile>
#include <QScopeGuard>

#include <memory>

namespace {

using namespace chatterino;

// The duration after which a sound is played we should try to stop the sound engine, hopefully
// returning the handle to idle letting the computer or monitors sleep
constexpr const auto STOP_AFTER_DURATION = std::chrono::seconds(30);

void miniaudioLogCallback(void *userData, ma_uint32 level, const char *pMessage)
{
    (void)userData;

    QString message{pMessage};

    switch (level)
    {
        case MA_LOG_LEVEL_DEBUG: {
            qCDebug(chatterinoSound).noquote()
                << "ma debug:  " << message.trimmed();
        }
        break;
        case MA_LOG_LEVEL_INFO: {
            qCDebug(chatterinoSound).noquote()
                << "ma info:   " << message.trimmed();
        }
        break;
        case MA_LOG_LEVEL_WARNING: {
            qCWarning(chatterinoSound).noquote()
                << "ma warning:" << message.trimmed();
        }
        break;
        case MA_LOG_LEVEL_ERROR: {
            qCWarning(chatterinoSound).noquote()
                << "ma error:  " << message.trimmed();
        }
        break;
        default: {
            qCWarning(chatterinoSound).noquote()
                << "ma unknown:" << message.trimmed();
        }
        break;
    }
}

}  // namespace

namespace chatterino {

MiniaudioBackend::MiniaudioBackend(bool keepEngineAlive_)
    : context(std::make_unique<ma_context>())
    , engine(std::make_unique<ma_engine>())
    , workGuard(boost::asio::make_work_guard(this->ioContext))
    , sleepTimer(this->ioContext)
    , keepEngineAlive(keepEngineAlive_)
{
    qCInfo(chatterinoSound) << "Initializing miniaudio sound backend";

    boost::asio::post(this->ioContext, [this] {
        ma_result result{};

        // We are leaking this log object on purpose
        auto *logger = new ma_log;

        result = ma_log_init(nullptr, logger);
        if (result != MA_SUCCESS)
        {
            qCWarning(chatterinoSound)
                << "Error initializing logger:" << result;
            this->state = State::Failed;
            return;
        }

        result = ma_log_register_callback(
            logger, ma_log_callback_init(miniaudioLogCallback, nullptr));
        if (result != MA_SUCCESS)
        {
            qCWarning(chatterinoSound)
                << "Error registering logger callback:" << result;
            this->state = State::Failed;
            return;
        }

        auto contextConfig = ma_context_config_init();
        contextConfig.pLog = logger;

        /// Initialize context
        result =
            ma_context_init(nullptr, 0, &contextConfig, this->context.get());
        if (result != MA_SUCCESS)
        {
            qCWarning(chatterinoSound)
                << "Error initializing context:" << result;
            this->state = State::Failed;
            return;
        }

        /// Initialize engine
        auto engineConfig = ma_engine_config_init();
        engineConfig.pContext = this->context.get();
        engineConfig.noAutoStart = MA_TRUE;

        result = ma_engine_init(&engineConfig, this->engine.get());
        if (result != MA_SUCCESS)
        {
            qCWarning(chatterinoSound)
                << "Error initializing engine:" << result;
            this->state = State::Failed;
            return;
        }

        if (this->keepEngineAlive)
        {
            // User has configured the "keep engine alive option"
            // We pre-start the engine to ensure it's available to play sounds as soon as possible.
            result = ma_engine_start(this->engine.get());
            if (result != MA_SUCCESS)
            {
                qCWarning(chatterinoSound)
                    << "Error pre-starting engine " << result;
            }
        }

        /// Initialize default ping sounds
        {
            // TODO: Can we optimize this?
            BenchmarkGuard b("init sounds");

            ma_uint32 soundFlags = 0;
            // Decode the sound during loading instead of during playback
            soundFlags |= MA_SOUND_FLAG_DECODE;
            // Disable pitch control (we don't use it, so this saves some performance)
            soundFlags |= MA_SOUND_FLAG_NO_PITCH;
            // Disable spatialization control, this brings the volume up to "normal levels"
            soundFlags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

            auto decoderConfig =
                ma_decoder_config_init(ma_format_f32, 0, 44100);
            // This must match the encoding format of our default ping sound
            decoderConfig.encodingFormat = ma_encoding_format_wav;

            for (const auto &[_, defaultSound] : highlights::defaultSounds())
            {
                BuiltInSound bis;

                /// Load default sound
                QFile defaultPingFile(defaultSound.resourcePath.sliced(3));
                if (!defaultPingFile.open(QIODevice::ReadOnly))
                {
                    qCWarning(chatterinoSound)
                        << "Error loading default ping sound"
                        << defaultSound.resourcePath;
                    this->state = State::Failed;
                    return;
                }
                bis.data = defaultPingFile.readAll();
                for (auto i = 0; i < NUM_SOUNDS; ++i)
                {
                    auto dec = std::make_unique<ma_decoder>();
                    auto snd = std::make_unique<ma_sound>();

                    result = ma_decoder_init_memory(
                        (void *)bis.data.data(), bis.data.size() * sizeof(char),
                        &decoderConfig, dec.get());
                    if (result != MA_SUCCESS)
                    {
                        qCWarning(chatterinoSound)
                            << "Error initializing default "
                               "ping decoder from memory:"
                            << result;
                        this->state = State::Failed;
                        return;
                    }

                    result = ma_sound_init_from_data_source(
                        this->engine.get(), dec.get(), soundFlags, nullptr,
                        snd.get());
                    if (result != MA_SUCCESS)
                    {
                        qCWarning(chatterinoSound)
                            << "Error initializing default sound from data "
                               "source:"
                            << result;
                        this->state = State::Failed;
                        return;
                    }

                    bis.decoders[i] = std::move(dec);
                    bis.sounds[i] = std::move(snd);
                }

                this->defaultPingSounds[defaultSound.resourcePath] =
                    std::move(bis);
            }
        }

        qCInfo(chatterinoSound) << "miniaudio sound system initialized";

        this->state = State::Initialized;
    });

    this->audioThread = std::make_unique<std::thread>([this] {
        auto guard = qScopeGuard([&] {
            this->stoppedFlag.set();
        });

        this->ioContext.run();
    });
    renameThread(*this->audioThread, "C2Miniaudio");
}

MiniaudioBackend::~MiniaudioBackend()
{
    this->state = State::Stopping;

    boost::asio::post(this->ioContext, [this] {
        for (const auto &[_, snds] : this->defaultPingSounds)
        {
            for (const auto &snd : snds.sounds)
            {
                ma_sound_uninit(snd.get());
            }
            for (const auto &dec : snds.decoders)
            {
                ma_decoder_uninit(dec.get());
            }
        }

        ma_engine_uninit(this->engine.get());
        ma_context_uninit(this->context.get());
    });

    this->workGuard.reset();
    this->sleepTimer.cancel();

    if (this->audioThread->joinable())
    {
        if (this->stoppedFlag.waitFor(std::chrono::seconds{1}))
        {
            this->audioThread->join();
            return;
        }

        qCWarning(chatterinoSound)
            << "Audio thread did not stop within 1 second";
    }
}

void MiniaudioBackend::play(const QUrl &sound)
{
    qInfo() << "XXX: Playing sound:" << sound;
    if (this->state != State::Initialized)
    {
        qCWarning(chatterinoSound) << "Can't play sound, sound controller "
                                      "is not initialized";
        return;
    }

    boost::asio::post(this->ioContext, [this, sound] {
        qCDebug(chatterinoSound) << "a";
        static size_t i = 0;

        this->tgPlay.guard();

        if (this->state != State::Initialized)
        {
            qCWarning(chatterinoSound) << "Can't play sound, sound controller "
                                          "is not initialized";
            return;
        }

        qCDebug(chatterinoSound) << "Starting engine";
        auto result = ma_engine_start(this->engine.get());
        if (result != MA_SUCCESS)
        {
            qCWarning(chatterinoSound) << "Error starting engine " << result;
            return;
        }

        qCDebug(chatterinoSound) << "hmm" << sound;
        if (sound.isLocalFile())
        {
            qCInfo(chatterinoSound) << "Playing local file" << sound;
            auto soundPath = sound.toLocalFile();
            result = ma_engine_play_sound(this->engine.get(),
                                          qPrintable(soundPath), nullptr);
            if (result != MA_SUCCESS)
            {
                qCWarning(chatterinoSound) << "Failed to play sound" << sound
                                           << soundPath << ":" << result;
            }
        }
        else
        {
            qCDebug(chatterinoSound) << "look for default pingsound" << sound;
            const auto defaultSoundIt = this->defaultPingSounds.find(sound);
            if (defaultSoundIt != this->defaultPingSounds.end())
            {
                qCInfo(chatterinoSound)
                    << "Found default ping sound for" << sound;
                // Play default sound, loaded from our resources in the constructor
                auto &snd = defaultSoundIt->second.sounds[++i % NUM_SOUNDS];
                ma_sound_seek_to_pcm_frame(snd.get(), 0);
                result = ma_sound_start(snd.get());
                if (result != MA_SUCCESS)
                {
                    qCWarning(chatterinoSound)
                        << "Failed to play default ping" << result;
                }
            }
            else
            {
                qCDebug(chatterinoSound)
                    << "did not found default ping sound" << sound;
                // The given resource didn't exist - do we hotload it?
            }
        }

        if (!this->keepEngineAlive)
        {
            this->sleepTimer.expires_after(STOP_AFTER_DURATION);
            this->sleepTimer.async_wait([this](const auto &ec) {
                if (ec)
                {
                    // Timer was most likely cancelled
                    return;
                }

                auto result = ma_engine_stop(this->engine.get());
                if (result != MA_SUCCESS)
                {
                    qCWarning(chatterinoSound)
                        << "Error stopping miniaudio engine " << result;
                    return;
                }
            });
        }
    });
}

}  // namespace chatterino
