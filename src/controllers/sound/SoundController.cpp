#include "controllers/sound/SoundController.hpp"

#include "common/QLogging.hpp"
#include "debug/Benchmark.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <QFile>

#include <limits>
#include <memory>

namespace chatterino {

// NUM_SOUNDS specifies how many simultaneous default ping sounds & decoders to create
constexpr const auto NUM_SOUNDS = 4;

SoundController::SoundController()
    : context(std::make_unique<ma_context>())
    , resourceManager(std::make_unique<ma_resource_manager>())
    , device(std::make_unique<ma_device>())
    , engine(std::make_unique<ma_engine>())
{
}

void SoundController::initialize(Settings &settings, Paths &paths)
{
    (void)(settings);
    (void)(paths);

    ma_result result{};

    /// Initialize context
    result = ma_context_init(nullptr, 0, nullptr, this->context.get());
    if (result != MA_SUCCESS)
    {
        qCWarning(chatterinoSound) << "Error initializing context:" << result;
        return;
    }

    /// Initialize resource manager
    auto resourceManagerConfig = ma_resource_manager_config_init();
    resourceManagerConfig.decodedFormat = ma_format_f32;
    // Use native channel count
    resourceManagerConfig.decodedChannels = 0;
    resourceManagerConfig.decodedSampleRate = 48000;

    result = ma_resource_manager_init(&resourceManagerConfig,
                                      this->resourceManager.get());
    if (result != MA_SUCCESS)
    {
        qCWarning(chatterinoSound)
            << "Error initializing resource manager:" << result;
        return;
    }

    /// Load default sound
    QFile defaultPingFile(":/sounds/ping2.wav");
    if (!defaultPingFile.open(QIODevice::ReadOnly))
    {
        qCWarning(chatterinoSound) << "Error loading default ping sound";
        return;
    }
    this->defaultPingData = defaultPingFile.readAll();

    /// Initialize a sound device
    auto deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.pDeviceID = nullptr;
    deviceConfig.playback.format = this->resourceManager->config.decodedFormat;
    deviceConfig.playback.channels = 0;
    deviceConfig.pulse.pStreamNamePlayback = "Chatterino MA";
    deviceConfig.sampleRate = this->resourceManager->config.decodedSampleRate;
    deviceConfig.dataCallback = ma_engine_data_callback_internal;
    deviceConfig.pUserData = this->engine.get();

    result =
        ma_device_init(this->context.get(), &deviceConfig, this->device.get());
    if (result != MA_SUCCESS)
    {
        qCWarning(chatterinoSound) << "Error initializing device:" << result;
        return;
    }

    result = ma_device_start(this->device.get());
    if (result != MA_SUCCESS)
    {
        qCWarning(chatterinoSound) << "Error starting device:" << result;
        return;
    }

    /// Initialize engine
    auto engineConfig = ma_engine_config_init();
    engineConfig.pResourceManager = this->resourceManager.get();
    engineConfig.pDevice = this->device.get();
    engineConfig.pContext = this->context.get();

    result = ma_engine_init(&engineConfig, this->engine.get());
    if (result != MA_SUCCESS)
    {
        qCWarning(chatterinoSound) << "Error initializing engine:" << result;
        return;
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

        auto decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 48000);
        // This must match the encoding format of our default ping sound
        decoderConfig.encodingFormat = ma_encoding_format_wav;

        for (auto i = 0; i < NUM_SOUNDS; ++i)
        {
            auto dec = std::make_unique<ma_decoder>();
            auto snd = std::make_unique<ma_sound>();

            result = ma_decoder_init_memory(
                (void *)this->defaultPingData.data(),
                this->defaultPingData.size() * sizeof(char), &decoderConfig,
                dec.get());
            if (result != MA_SUCCESS)
            {
                qCWarning(chatterinoSound)
                    << "Error initializing default ping decoder from memory:"
                    << result;
                return;
            }

            result = ma_sound_init_from_data_source(
                this->engine.get(), dec.get(), soundFlags, nullptr, snd.get());
            if (result != MA_SUCCESS)
            {
                qCWarning(chatterinoSound)
                    << "Error initializing default sound from data source:"
                    << result;
                return;
            }

            this->defaultPingDecoders.emplace_back(std::move(dec));
            this->defaultPingSounds.emplace_back(std::move(snd));
        }
    }

    qCInfo(chatterinoSound) << "miniaudio sound system initialized";

    this->initialized = true;
}

SoundController::~SoundController()
{
    // NOTE: This destructor is never called because the `runGui` function calls _exit before that happens
    // I have manually called the destructor prior to _exit being called to ensure this logic is sound

    for (const auto &snd : this->defaultPingSounds)
    {
        ma_sound_uninit(snd.get());
    }
    for (const auto &dec : this->defaultPingDecoders)
    {
        ma_decoder_uninit(dec.get());
    }

    ma_engine_uninit(this->engine.get());
    ma_device_uninit(this->device.get());
    ma_resource_manager_uninit(this->resourceManager.get());
    ma_context_uninit(this->context.get());
}

void SoundController::play(const QUrl &sound)
{
    static size_t i = 0;

    this->tgPlay.guard();

    if (!this->initialized)
    {
        qCWarning(chatterinoSound) << "Can't play sound, sound controller "
                                      "didn't initialize correctly";
        return;
    }

    if (sound.isLocalFile())
    {
        auto soundPath = sound.toLocalFile();
        auto result = ma_engine_play_sound(this->engine.get(),
                                           qPrintable(soundPath), nullptr);
        if (result != MA_SUCCESS)
        {
            qCWarning(chatterinoSound) << "Failed to play sound" << sound
                                       << soundPath << ":" << result;
        }

        return;
    }

    // Play default sound, loaded from our resources in the constructor
    auto &snd = this->defaultPingSounds[++i % NUM_SOUNDS];
    ma_sound_seek_to_pcm_frame(snd.get(), 0);
    auto result = ma_sound_start(snd.get());
    if (result != MA_SUCCESS)
    {
        qCWarning(chatterinoSound) << "Failed to play default ping" << result;
    }
}

}  // namespace chatterino
