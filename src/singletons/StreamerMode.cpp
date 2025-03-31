#include "singletons/StreamerMode.hpp"

#include "Application.hpp"
#include "common/Literals.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Settings.hpp"
#include "util/PostToThread.hpp"

#include <QAbstractEventDispatcher>
#include <QDebug>
#include <QProcess>
#include <QThread>
#include <QTimer>

#ifdef Q_OS_WIN
// clang-format off
#    include <Windows.h>
#    include <VersionHelpers.h>
#    include <WtsApi32.h>
// clang-format on
#endif

#include <atomic>

namespace {

using namespace chatterino;
using namespace literals;

/// Number of timeouts to skip if nothing called `isEnabled` in the meantime.
constexpr uint8_t SKIPPED_TIMEOUTS = 5;

const QStringList &broadcastingBinaries()
{
#ifdef USEWINSDK
    static QStringList bins = {
        u"obs.exe"_s,         u"obs64.exe"_s,        u"PRISMLiveStudio.exe"_s,
        u"XSplit.Core.exe"_s, u"TwitchStudio.exe"_s, u"vMix64.exe"_s,
    };
#else
    static QStringList bins = {
        u"obs"_s,
        u"Twitch Studio"_s,
        u"Streamlabs Desktop"_s,
    };
#endif
    return bins;
}

bool isBroadcasterSoftwareActive()
{
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    static bool shouldShowTimeoutWarning = true;
    static bool shouldShowWarning = true;

    QProcess p;
    p.start("pgrep", {"-xi", broadcastingBinaries().join("|")},
            QIODevice::NotOpen);

    if (p.waitForFinished(1000) && p.exitStatus() == QProcess::NormalExit)
    {
        return (p.exitCode() == 0);
    }

    // Fallback to false and showing a warning

    switch (p.error())
    {
        case QProcess::Timedout: {
            qCWarning(chatterinoStreamerMode) << "pgrep execution timed out!";
            if (shouldShowTimeoutWarning)
            {
                shouldShowTimeoutWarning = false;

                postToThread([] {
                    getApp()->getTwitch()->addGlobalSystemMessage(
                        "Streamer Mode is set to Automatic, but pgrep timed "
                        "out. This can happen if your system lagged at the "
                        "wrong moment. If Streamer Mode continues to not work, "
                        "you can manually set it to Enabled or Disabled in the "
                        "Settings.");
                });
            }
        }
        break;

        default: {
            qCWarning(chatterinoStreamerMode)
                << "pgrep execution failed:" << p.error();

            if (shouldShowWarning)
            {
                shouldShowWarning = false;

                postToThread([] {
                    getApp()->getTwitch()->addGlobalSystemMessage(
                        "Streamer Mode is set to Automatic, but pgrep is "
                        "missing. "
                        "Install it to fix the issue or set Streamer Mode to "
                        "Enabled or Disabled in the Settings.");
                });
            }
        }
        break;
    }

    if (!p.waitForFinished(1000))
    {
        qCWarning(chatterinoStreamerMode) << "Force-killing pgrep";
        p.kill();
    }

    return false;
#elif defined(Q_OS_WIN)
    if (!IsWindowsVistaOrGreater())
    {
        return false;
    }

    WTS_PROCESS_INFO *pProcessInfo = nullptr;
    DWORD dwProcCount = 0;

    if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo,
                              &dwProcCount))
    {
        //Go through all processes retrieved
        for (DWORD i = 0; i < dwProcCount; i++)
        {
            QStringView processName(pProcessInfo[i].pProcessName);

            if (broadcastingBinaries().contains(processName))
            {
                WTSFreeMemory(pProcessInfo);
                return true;
            }
        }
    }

    if (pProcessInfo)
    {
        WTSFreeMemory(pProcessInfo);
    }

#else
#    warning Unsupported OS: Broadcasting software can\'t be detected
#endif
    return false;
}

}  // namespace

namespace chatterino {

using namespace std::chrono_literals;

class StreamerModePrivate
{
public:
    StreamerModePrivate(StreamerMode *parent_);
    ~StreamerModePrivate();
    StreamerModePrivate(const StreamerModePrivate &) = delete;
    StreamerModePrivate(StreamerModePrivate &&) = delete;
    StreamerModePrivate &operator=(const StreamerModePrivate &) = delete;
    StreamerModePrivate &operator=(StreamerModePrivate &&) = delete;

    [[nodiscard]] bool isEnabled() const;

    void start();

private:
    void settingChanged(StreamerModeSetting value);
    void setEnabled(bool enabled);

    void check();

    StreamerMode *parent_;
    pajlada::Signals::SignalHolder settingConnections_;

    QThread thread_;
    QTimer *timer_;

    std::atomic<bool> enabled_ = false;
    mutable std::atomic<uint8_t> timeouts_ = 0;
    StreamerModeSetting currentSetting_ = StreamerModeSetting::Disabled;
};

StreamerMode::StreamerMode()
    : private_(new StreamerModePrivate(this))
{
}

StreamerMode::~StreamerMode() = default;

void StreamerMode::updated(bool enabled)
{
    this->changed(enabled);
}

bool StreamerMode::isEnabled() const
{
    return this->private_->isEnabled();
}

bool StreamerMode::shouldHideModActions() const
{
    return getSettings()->streamerModeHideModActions && this->isEnabled();
}

bool StreamerMode::shouldHideRestrictedUsers() const
{
    return getSettings()->streamerModeHideRestrictedUsers && this->isEnabled();
}

void StreamerMode::start()
{
    this->private_->start();
}

StreamerModePrivate::StreamerModePrivate(StreamerMode *parent)
    : parent_(parent)
    , timer_(new QTimer(&this->thread_))
{
    this->thread_.setObjectName("StreamerMode");
    this->timer_->moveToThread(&this->thread_);
    QObject::connect(this->timer_, &QTimer::timeout, [this] {
        auto timeouts =
            this->timeouts_.fetch_add(1, std::memory_order::relaxed);
        if (timeouts < SKIPPED_TIMEOUTS)
        {
            return;
        }
        this->timeouts_.store(0, std::memory_order::relaxed);
        this->check();
    });

    getSettings()->enableStreamerMode.connect(
        [this](auto value) {
            QMetaObject::invokeMethod(this->thread_.eventDispatcher(), [this,
                                                                        value] {
                this->settingChanged(static_cast<StreamerModeSetting>(value));
            });
        },
        this->settingConnections_);

    QObject::connect(&this->thread_, &QThread::started, [this] {
        this->settingChanged(getSettings()->enableStreamerMode.getEnum());
    });
}

void StreamerModePrivate::start()
{
    this->thread_.start();
}

StreamerModePrivate::~StreamerModePrivate()
{
    this->timer_->deleteLater();
    this->timer_ = nullptr;
    this->thread_.quit();
    if (!this->thread_.wait(500))
    {
        qCWarning(chatterinoStreamerMode)
            << "Failed waiting for thread, terminating it";
        this->thread_.terminate();
    }
}

bool StreamerModePrivate::isEnabled() const
{
    this->timeouts_.store(SKIPPED_TIMEOUTS, std::memory_order::relaxed);
    return this->enabled_.load(std::memory_order::relaxed);
}

void StreamerModePrivate::setEnabled(bool enabled)
{
    if (enabled == this->enabled_.load(std::memory_order::relaxed))
    {
        return;
    }

    this->enabled_.store(enabled, std::memory_order::relaxed);
    this->parent_->updated(enabled);
}

void StreamerModePrivate::settingChanged(StreamerModeSetting value)
{
    if (value == this->currentSetting_)
    {
        return;
    }
    this->currentSetting_ = value;

    // in all cases: timer_ must be invoked from the correct thread
    switch (this->currentSetting_)
    {
        case StreamerModeSetting::Disabled: {
            this->setEnabled(false);
            QMetaObject::invokeMethod(this->timer_, &QTimer::stop);
        }
        break;
        case StreamerModeSetting::Enabled: {
            this->setEnabled(true);
            QMetaObject::invokeMethod(this->timer_, &QTimer::stop);
        }
        break;
        case StreamerModeSetting::DetectStreamingSoftware: {
            QMetaObject::invokeMethod(this->timer_, [this] {
                if (!this->timer_->isActive())
                {
                    this->timer_->start(20s);
                    this->check();
                }
            });
        }
        break;
        default:
            assert(false && "Unexpected setting");
            break;
    }
}

void StreamerModePrivate::check()
{
    this->setEnabled(isBroadcasterSoftwareActive());
}

}  // namespace chatterino
