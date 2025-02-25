#include "controllers/sound/MiniaudioBackend.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "debug/Benchmark.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/RenameThread.hpp"
#include "widgets/Window.hpp"

#include <boost/asio/executor_work_guard.hpp>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <QFile>
#include <QScopeGuard>

#include <limits>
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

// NUM_SOUNDS specifies how many simultaneous default ping sounds & decoders to create
constexpr const auto NUM_SOUNDS = 4;

MiniaudioBackend::MiniaudioBackend()
    : context(std::make_unique<ma_context>())
    , engine(std::make_unique<ma_engine>())
    , workGuard(boost::asio::make_work_guard(this->ioContext))
    , sleepTimer(this->ioContext)
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
            return;
        }

        result = ma_log_register_callback(
            logger, ma_log_callback_init(miniaudioLogCallback, nullptr));
        if (result != MA_SUCCESS)
        {
            qCWarning(chatterinoSound)
                << "Error registering logger callback:" << result;
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

        /// Initialize engine
        auto engineConfig = ma_engine_config_init();
        engineConfig.pContext = this->context.get();
        engineConfig.noAutoStart = MA_TRUE;

        result = ma_engine_init(&engineConfig, this->engine.get());
        if (result != MA_SUCCESS)
        {
            qCWarning(chatterinoSound)
                << "Error initializing engine:" << result;
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

            auto decoderConfig =
                ma_decoder_config_init(ma_format_f32, 0, 48000);
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
                    qCWarning(chatterinoSound) << "Error initializing default "
                                                  "ping decoder from memory:"
                                               << result;
                    return;
                }

                result = ma_sound_init_from_data_source(this->engine.get(),
                                                        dec.get(), soundFlags,
                                                        nullptr, snd.get());
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
    // NOTE: This destructor is never called because the `runGui` function calls _exit before that happens
    // I have manually called the destructor prior to _exit being called to ensure this logic is sound

    boost::asio::post(this->ioContext, [this] {
        for (const auto &snd : this->defaultPingSounds)
        {
            ma_sound_uninit(snd.get());
        }
        for (const auto &dec : this->defaultPingDecoders)
        {
            ma_decoder_uninit(dec.get());
        }

        ma_engine_uninit(this->engine.get());
        ma_context_uninit(this->context.get());

        this->workGuard.reset();
    });

    if (!this->audioThread->joinable())
    {
        qCWarning(chatterinoSound) << "Audio thread not joinable";
        return;
    }

    if (this->stoppedFlag.waitFor(std::chrono::seconds{1}))
    {
        this->audioThread->join();
        return;
    }

    qCWarning(chatterinoSound) << "Audio thread did not stop within 1 second";
    this->audioThread->detach();
}

void MiniaudioBackend::play(const QUrl &sound)
{
    boost::asio::post(this->ioContext, [this, sound] {
        static size_t i = 0;

        this->tgPlay.guard();

        if (!this->initialized)
        {
            qCWarning(chatterinoSound) << "Can't play sound, sound controller "
                                          "didn't initialize correctly";
            return;
        }

        auto result = ma_engine_start(this->engine.get());
        if (result != MA_SUCCESS)
        {
            qCWarning(chatterinoSound) << "Error starting engine " << result;
            return;
        }

        if (sound.isLocalFile())
        {
            auto soundPath = sound.toLocalFile();
            result = ma_engine_play_sound(this->engine.get(),
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
        result = ma_sound_start(snd.get());
        if (result != MA_SUCCESS)
        {
            qCWarning(chatterinoSound)
                << "Failed to play default ping" << result;
        }

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
    });
}

}  // namespace chatterino
