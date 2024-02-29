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
    QProcess p;
    p.start("pgrep", {"-x", broadcastingBinaries().join("|")},
            QIODevice::NotOpen);

    if (p.waitForFinished(1000) && p.exitStatus() == QProcess::NormalExit)
    {
        return (p.exitCode() == 0);
    }

    // Fallback to false and showing a warning

    static bool shouldShowWarning = true;
    if (shouldShowWarning)
    {
        shouldShowWarning = false;

        postToThread([] {
            getApp()->twitch->addGlobalSystemMessage(
                "Streamer Mode is set to Automatic, but pgrep is missing. "
                "Install it to fix the issue or set Streamer Mode to "
                "Enabled or Disabled in the Settings.");
        });
    }

    qCWarning(chatterinoStreamerMode) << "pgrep execution timed out!";
    return false;
#elif defined(Q_OS_WIN)  // TODO(C++ 23): use elifdef
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
#    warning Unsupported OS: Broadcasting software can't be detected
#endif
    return false;
}

class SingleConnection
{
public:
    // NOLINTNEXTLINE(readability-identifier-naming)
    void emplace_back(auto &&conn)
    {
        this->conn_ = std::forward<decltype(conn)>(conn);
    }

private:
    std::unique_ptr<pajlada::Signals::ScopedConnection> conn_;
};

}  // namespace

namespace chatterino {

using namespace std::chrono_literals;

class StreamerModePrivate
{
public:
    StreamerModePrivate(StreamerMode *parent_);

    [[nodiscard]] bool isEnabled() const;

private:
    void settingChanged(StreamerModeSetting value);
    void setEnabled(bool enabled);

    void check();

    StreamerMode *parent_;
    SingleConnection settingConnection_;

    QTimer timer_;
    QThread thread_;

    std::atomic_bool enabled_ = false;
    mutable std::atomic_uint8_t timeouts_ = 0;
    StreamerModeSetting prevValue_ = StreamerModeSetting::Disabled;
};

StreamerMode::StreamerMode()
    : private_(new StreamerModePrivate(this))
{
}

StreamerMode::~StreamerMode() = default;

void StreamerMode::updated()
{
    this->changed(this->isEnabled());
}

bool StreamerMode::isEnabled() const
{
    return this->private_->isEnabled();
}

StreamerModePrivate::StreamerModePrivate(StreamerMode *parent)
    : parent_(parent)
{
    this->thread_.start();
    this->timer_.moveToThread(&this->thread_);
    QObject::connect(&this->timer_, &QTimer::timeout, [this] {
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
        this->settingConnection_);
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
    this->parent_->updated();
}

void StreamerModePrivate::settingChanged(StreamerModeSetting value)
{
    if (value == this->prevValue_)
    {
        return;
    }
    this->prevValue_ = value;

    switch (this->prevValue_)
    {
        case StreamerModeSetting::Disabled:
            this->setEnabled(false);
            [[fallthrough]];
        case StreamerModeSetting::Enabled:
            this->setEnabled(true);
            this->timer_.stop();
        case StreamerModeSetting::DetectStreamingSoftware: {
            if (!this->timer_.isActive())
            {
                this->timer_.start(20s);
                this->check();
            }
        }
        break;
        default:
            assert(false && "Unexpected setting");
    }
}

void StreamerModePrivate::check()
{
    this->setEnabled(isBroadcasterSoftwareActive());
}

}  // namespace chatterino
