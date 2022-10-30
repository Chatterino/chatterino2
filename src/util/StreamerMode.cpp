#include "StreamerMode.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Window.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/splits/Split.hpp"

#ifdef USEWINSDK
#    include <Windows.h>

#    include <VersionHelpers.h>
#    include <WtsApi32.h>
#    pragma comment(lib, "Wtsapi32.lib")
#endif

#include <QProcess>

namespace chatterino {

constexpr int cooldownInS = 10;

bool shouldShowWarning = true;

const QStringList &broadcastingBinaries()
{
#ifdef USEWINSDK
    static QStringList bins = {
        "obs.exe",         "obs64.exe",        "PRISMLiveStudio.exe",
        "XSplit.Core.exe", "TwitchStudio.exe", "vMix64.exe"};
#else
    static QStringList bins = {"obs", "Twitch Studio", "Streamlabs Desktop"};
#endif
    return bins;
}

bool isInStreamerMode()
{
    switch (getSettings()->enableStreamerMode.getEnum())
    {
        case StreamerModeSetting::Enabled:
            return true;
        case StreamerModeSetting::Disabled:
            return false;
        case StreamerModeSetting::DetectStreamingSoftware:

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)

            static bool cache = false;
            static QDateTime time = QDateTime();

            if (time.isValid() &&
                time.addSecs(cooldownInS) > QDateTime::currentDateTime())
            {
                return cache;
            }
            time = QDateTime::currentDateTime();

            QProcess p;
            p.start("pgrep", {"-x", broadcastingBinaries().join("|")},
                    QIODevice::NotOpen);

            if (p.waitForFinished(1000) &&
                p.exitStatus() == QProcess::NormalExit)
            {
                cache = (p.exitCode() == 0);
                return (p.exitCode() == 0);
            }

            // Fallback to false and showing a warning

            if (shouldShowWarning)
            {
                shouldShowWarning = false;

                getApp()->twitch->addGlobalSystemMessage(
                    "Streamer Mode is set to Automatic, but pgrep is missing. "
                    "Install it to fix the issue or set Streamer Mode to "
                    "Enabled or Disabled in the Settings.");
            }

            qCWarning(chatterinoStreamerMode) << "pgrep execution timed out!";

            cache = false;
            return false;
#endif

#ifdef USEWINSDK
            if (!IsWindowsVistaOrGreater())
            {
                return false;
            }
            static bool cache = false;
            static QDateTime time = QDateTime();

            if (time.isValid() &&
                time.addSecs(cooldownInS) > QDateTime::currentDateTime())
            {
                return cache;
            }
            time = QDateTime::currentDateTime();

            WTS_PROCESS_INFO *pWPIs = nullptr;
            DWORD dwProcCount = 0;

            if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, NULL, 1,
                                      &pWPIs, &dwProcCount))
            {
                //Go through all processes retrieved
                for (DWORD i = 0; i < dwProcCount; i++)
                {
                    QString processName = QString::fromUtf16(
                        reinterpret_cast<char16_t *>(pWPIs[i].pProcessName));

                    if (broadcastingBinaries().contains(processName))
                    {
                        cache = true;
                        return true;
                    }
                }
            }

            if (pWPIs)
            {
                WTSFreeMemory(pWPIs);
            }

            cache = false;
#endif
            return false;
    }
    return false;
}

}  // namespace chatterino
