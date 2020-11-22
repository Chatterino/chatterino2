#include "StreamerMode.hpp"

#include "singletons/Settings.hpp"

#ifdef USEWINSDK
#    include <Windows.h>

#    include <VersionHelpers.h>
#    include <WtsApi32.h>
#    pragma comment(lib, "Wtsapi32.lib")
#endif

namespace chatterino {

#ifdef USEWINSDK
constexpr int cooldownInS = 10;
#endif

const QStringList &broadcastingBinaries()
{
#ifdef USEWINSDK
    static QStringList bins = {"obs.exe", "obs64.exe"};
#else
    static QStringList bins = {};
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
    }

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

    if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, NULL, 1, &pWPIs,
                              &dwProcCount))
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

}  // namespace chatterino
